// CureBleProtocolClient.swift
// Minimal CoreBluetooth skeleton for unlock parity (NO OTA)

import Foundation
import CoreBluetooth

// NOTE: This file expects the libsecp256k1 C library to be available and bridged to Swift
// via a module named `secp256k1`. Make sure your Xcode project links libsecp256k1 and
// exposes the header in a bridging header if needed.
// The computeSignature implementation below uses the C functions from that library.

final class CureBleProtocolClient: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    // Exact UUIDs required
    private let nusService = CBUUID(string: "6e400001-b5a3-f393-e0a9-e50e24dcca9e")
    private let rxCharUUID = CBUUID(string: "6e400002-b5a3-f393-e0a9-e50e24dcca9e") // write
    private let txCharUUID = CBUUID(string: "6e400003-b5a3-f393-e0a9-e50e24dcca9e") // notify

    private var central: CBCentralManager!
    private(set) var peripheral: CBPeripheral?
    private var rxCharacteristic: CBCharacteristic?
    private var txCharacteristic: CBCharacteristic?

    private(set) var protocolReady: Bool = false

    // RX buffering / lines
    private var rxBuffer = Data()
    private var pendingLines: [String] = []
    private let pendingLinesQueue = DispatchQueue(label: "cure.pendingLinesQueue")

    // Write confirmation continuation
    private var writeContinuationLock = DispatchQueue(label: "cure.writeContLock")
    private var pendingWriteContinuation: CheckedContinuation<Bool, Never>?

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)
    }

    // MARK: - Public connect API
    func connect(to peripheral: CBPeripheral) {
        self.peripheral = peripheral
        peripheral.delegate = self
        central.connect(peripheral, options: nil)
    }

    // MARK: - CBCentralManagerDelegate
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        // handle states as needed
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([nusService])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        protocolReady = false
    }

    // MARK: - CBPeripheralDelegate (discovery)
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        for service in services {
            if service.uuid == nusService {
                peripheral.discoverCharacteristics([rxCharUUID, txCharUUID], for: service)
                return
            }
        }
        // NUS not found
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let chars = service.characteristics else { return }
        var foundRX: CBCharacteristic?
        var foundTX: CBCharacteristic?
        for c in chars {
            if c.uuid == rxCharUUID { foundRX = c }
            if c.uuid == txCharUUID { foundTX = c }
        }
        guard let rx = foundRX, let tx = foundTX else { return }
        rxCharacteristic = rx
        txCharacteristic = tx

        // enable notifications on TX; wait for didUpdateNotificationStateFor to mark ready
        peripheral.setNotifyValue(true, for: tx)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        if characteristic.uuid == txCharUUID && characteristic.isNotifying {
            protocolReady = true
        } else {
            protocolReady = false
        }
    }

    // MARK: - Notification / line parser
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic.uuid == txCharUUID, let data = characteristic.value, !data.isEmpty else { return }
        appendAndProcess(data: data)
    }

    private func appendAndProcess(data: Data) {
        pendingLinesQueue.sync {
            rxBuffer.append(data)
            while let idx = rxBuffer.firstIndex(of: 0x0A) { // '\n'
                let lineData = rxBuffer.subdata(in: 0...idx)
                rxBuffer.removeSubrange(0...idx)
                if let s = String(data: lineData, encoding: .isoLatin1) {
                    let trimmed = s.trimmingCharacters(in: CharacterSet(charactersIn: "\r\n"))
                    pendingLines.append(trimmed)
                }
            }
        }
    }

    private func dequeueLine() -> String? {
        return pendingLinesQueue.sync {
            if pendingLines.isEmpty { return nil }
            return pendingLines.removeFirst()
        }
    }

    func waitForLine(matching predicate: @escaping (String) -> Bool, timeout: TimeInterval) async -> String? {
        let deadline = Date().addingTimeInterval(timeout)
        while Date() < deadline {
            if let line = dequeueLine() {
                if predicate(line) { return line }
            } else {
                try? await Task.sleep(nanoseconds: 20_000_000)
            }
        }
        return nil
    }

    // MARK: - write helpers
    func waitForWriteConfirmation(timeout: TimeInterval = 3.0) async -> Bool {
        return await withCheckedContinuation { (cont: CheckedContinuation<Bool, Never>) in
            writeContinuationLock.sync {
                pendingWriteContinuation = cont
            }
            Task {
                try? await Task.sleep(nanoseconds: UInt64(timeout * 1_000_000_000))
                writeContinuationLock.sync {
                    if let cont = pendingWriteContinuation {
                        pendingWriteContinuation = nil
                        cont.resume(returning: false)
                    }
                }
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        writeContinuationLock.sync {
            if let cont = pendingWriteContinuation {
                pendingWriteContinuation = nil
                cont.resume(returning: (error == nil))
            }
        }
    }

    func sendLine(_ cmd: String) async -> Bool {
        guard protocolReady, let rx = rxCharacteristic, let p = peripheral else { return false }
        let line = cmd.hasSuffix("\n") ? cmd : (cmd + "\r\n")
        guard let data = line.data(using: .isoLatin1) else { return false }
        p.writeValue(data, for: rx, type: .withResponse)
        let ok = await waitForWriteConfirmation(timeout: 3.0)
        return ok
    }

    func writeLine(_ cmd: String, expect predicate: @escaping (String) -> Bool, timeout: TimeInterval = 6.0) async -> String? {
        let wrote = await sendLine(cmd)
        if !wrote { return nil }
        return await waitForLine(matching: predicate, timeout: timeout)
    }

    // MARK: - Unlock flow helpers
    func requestChallenge() async -> String? {
        let hexPredicate: (String) -> Bool = { line in
            let pattern = "^[0-9A-Fa-f]{64}$"
            return line.range(of: pattern, options: .regularExpression) != nil
        }
        _ = await sendLine("challenge")
        if let found = await waitForLine(matching: hexPredicate, timeout: 8.0) {
            return found.uppercased()
        }
        return nil
    }

    // computeSignature using libsecp256k1 (expects library to be linked)
    func computeSignature(for challenge: String) -> String {
        // Debug: log input
        print("computeSignature: challenge input=\(challenge)")
        let cleaned = challenge.replacingOccurrences(of: "[^0-9A-Fa-f]", with: "", options: .regularExpression)
        guard cleaned.count == 64, let msg = Data(hex: cleaned) else {
            print("computeSignature: invalid challenge length")
            return ""
        }
        print("computeSignature: parsed challenge bytes len=\(msg.count)")

        // private key from Android CureCrypto. Must match exactly.
        let privHex = "E40783F681A5BB852CAB1E106B6641EFB43C1923C1EBE25CA36865CDFAB06548"
        guard let priv = Data(hex: privHex) else { return "" }

        // Use secp256k1 to sign the 32-byte message (no extra hashing; mirrors Android HASH_CHALLENGE_BEFORE_SIGN=false)
        // This implementation requires a bridged libsecp256k1; if absent, this will fail at link time.

        // Create context
        guard let ctx = secp256k1_context_create(UInt32(SECP256K1_CONTEXT_SIGN)) else {
            print("computeSignature: failed to create secp context")
            return ""
        }
        defer { secp256k1_context_destroy(ctx) }

        var signature = secp256k1_ecdsa_signature()
        let signed: Bool = msg.withUnsafeBytes { (msgPtr: UnsafeRawBufferPointer) -> Bool in
            priv.withUnsafeBytes { (privPtr: UnsafeRawBufferPointer) -> Bool in
                guard let m = msgPtr.baseAddress, let p = privPtr.baseAddress else { return false }
                let rc = secp256k1_ecdsa_sign(ctx, &signature, m.assumingMemoryBound(to: UInt8.self), p.assumingMemoryBound(to: UInt8.self), nil, nil)
                return rc == 1
            }
        }
        if !signed {
            print("computeSignature: secp sign failed")
            return ""
        }

        // Enforce low-S normalization
        var sigNorm = signature
        let _ = secp256k1_ecdsa_signature_normalize(ctx, &sigNorm, &signature)

        // serialize compact r||s (64 bytes)
        var compact = [UInt8](repeating: 0, count: 64)
        let ok = secp256k1_ecdsa_signature_serialize_compact(ctx, &compact, &sigNorm)
        if ok != 1 {
            print("computeSignature: serialize failed")
            return ""
        }
        let sigData = Data(compact)
        let sigHex = sigData.hexString()
        print("computeSignature: produced signature hex len=\(sigHex.count) final=\(sigHex)")
        return sigHex
    }

    func sendUnlockResponse(signatureHex: String) async -> Bool {
        guard let p = peripheral, let rx = rxCharacteristic else { return false }
        print("sendUnlockResponse: signatureHex len=\(signatureHex.count)")
        let payload = "response=\(signatureHex)\r\n"
        guard let data = payload.data(using: .isoLatin1) else { return false }
        let maxChunk = p.maximumWriteValueLength(for: .withResponse)
        var offset = 0
        while offset < data.count {
            let len = min(maxChunk, data.count - offset)
            let chunk = data.subdata(in: offset..<(offset+len))
            p.writeValue(chunk, for: rx, type: .withResponse)
            let wroteOk = await waitForWriteConfirmation(timeout: 3.0)
            if !wroteOk { return false }
            offset += len
        }
        let okPredicate: (String) -> Bool = { line in
            let l = line.uppercased()
            return l == "OK" || l.contains("UNLOCKED")
        }
        if let _ = await waitForLine(matching: okPredicate, timeout: 20.0) {
            return true
        }
        return false
    }

    func unlockFlow() async -> Bool {
        guard let challenge = await requestChallenge() else { return false }
        let sig = computeSignature(for: challenge)
        if sig.isEmpty { return false }
        let ok = await sendUnlockResponse(signatureHex: sig)
        return ok
    }
}

// MARK: - Helpers (hex <-> Data)
extension Data {
    init?(hex: String) {
        let s = hex.count % 2 == 0 ? hex : "0" + hex
        var data = Data(capacity: s.count/2)
        var idx = s.startIndex
        while idx < s.endIndex {
            let next = s.index(idx, offsetBy: 2)
            let byteStr = s[idx..<next]
            if let b = UInt8(byteStr, radix: 16) {
                data.append(b)
            } else { return nil }
            idx = next
        }
        self = data
    }
    func hexString() -> String {
        return self.map { String(format: "%02x", $0) }.joined()
    }
}

// Bridging declarations for libsecp256k1 (these symbols must be available at link time)
import secp256k1

// The C types/functions used (from libsecp256k1) are assumed available via the module:
// secp256k1_context_create, secp256k1_context_destroy,
// secp256k1_ecdsa_sign, secp256k1_ecdsa_signature_normalize,
// secp256k1_ecdsa_signature_serialize_compact

```
