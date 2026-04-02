package com.example.curebaseupdater

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import kotlinx.coroutines.suspendCancellableCoroutine
import java.nio.charset.Charset
import java.util.UUID
import kotlin.coroutines.resume

private const val TAG = "CURE_UPDATER"

// Nordic UART Service UUIDs (NUS)
private val UART_SERVICE_UUID: UUID = UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
private val UART_RX_UUID: UUID = UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
private val UART_TX_UUID: UUID = UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e")

/**
 * BleUartClient (CureBase Updater)
 *
 * FIXES included:
 * - Small ASCII commands (challenge/gethardware/getbuild) rely on NOTIFY ACK (HEX + OK), NOT on onCharacteristicWrite.
 * - For this device, onCharacteristicWrite is unreliable / delayed (can end in status=133). Therefore we default to WRITE_TYPE_NO_RESPONSE.
 * - Burst writer uses watchdog "assume-sent" pacing, plus a short cooldown, and BUSY(201) gating to avoid hammering the stack.
 *
 * This matches your logs: device replies quickly via NOTIFY, but ATT write response is not timely -> Android reports BUSY then 133.
 */
class BleUartClient(private val context: Context) {

    // --- IMPORTANT: force NO_RESPONSE to avoid "with response" pipeline stalls that end in BUSY/133 on some devices ---
    private val FORCE_WRITE_NO_RESPONSE = true

    init {
        try {
            fun getCode(name: String): String {
                return try {
                    val cls = Class.forName("android.bluetooth.BluetoothStatusCodes")
                    val f = cls.getField(name)
                    f.getInt(null).toString()
                } catch (_: Throwable) {
                    "n/a"
                }
            }
            val msg = "BluetoothStatusCodes: SUCCESS=${getCode("SUCCESS")} " +
                    "ERROR_GATT_WRITE_REQUEST_BUSY=${getCode("ERROR_GATT_WRITE_REQUEST_BUSY")} " +
                    "ERROR_GATT=${getCode("ERROR_GATT")}"
            Log.d(TAG, msg)
        } catch (t: Throwable) {
            Log.d(TAG, "BluetoothStatusCodes logging init failed: ${t.message}")
        }
    }

    private val btManager: BluetoothManager? =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager
    private val adapter: BluetoothAdapter? = btManager?.adapter
    private val scanner: BluetoothLeScanner? = adapter?.bluetoothLeScanner

    private var gatt: BluetoothGatt? = null
    private var rxChar: BluetoothGattCharacteristic? = null
    private var txChar: BluetoothGattCharacteristic? = null

    private val onLineListeners = mutableListOf<(String) -> Unit>()
    private val onStateListeners = mutableListOf<(String) -> Unit>()

    private val mainHandler = Handler(Looper.getMainLooper())

    @Volatile private var connectAttemptId: Long = 0L
    @Volatile private var cleanupToken: Long = 0L

    private fun hasBtScan(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
        } else true
    }

    private fun hasBtConnect(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED
        } else true
    }

    private inline fun <T> safeBt(tag: String, fallback: T, block: () -> T): T {
        return try {
            block()
        } catch (se: SecurityException) {
            Log.w(TAG, "SecurityException in $tag: ${se.message}")
            emitState("Permission missing for BLE op: $tag")
            fallback
        } catch (t: Throwable) {
            Log.w(TAG, "Exception in $tag: ${t.message}")
            fallback
        }
    }

    fun addOnLineListener(cb: (String) -> Unit) { onLineListeners.add(cb) }
    fun addOnStateListener(cb: (String) -> Unit) { onStateListeners.add(cb) }

    private fun emitLine(s: String) { for (cb in onLineListeners.toList()) cb(s) }
    private fun emitState(s: String) { for (cb in onStateListeners.toList()) cb(s) }

    // ---------- Scan plumbing ----------
    private var scanResultHandler: ((ScanResult) -> Unit)? = null
    private var isScanning: Boolean = false

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            if (gatt != null) return
            result?.let { r ->
                val rec = r.scanRecord
                val uuids = rec?.serviceUuids?.joinToString { it.uuid.toString() } ?: "-"
                val mfgCount = rec?.manufacturerSpecificData?.size() ?: 0
                val addr = r.device?.address ?: "?"
                val name = r.device?.name ?: rec?.deviceName
                Log.d(TAG, "SCAN hit: addr=$addr name=${name ?: "(no name)"} rssi=${r.rssi} uuids=$uuids mfgCount=$mfgCount")
                scanResultHandler?.invoke(r)
            }
        }

        override fun onBatchScanResults(results: MutableList<ScanResult>?) {
            Log.d(TAG, "SCAN batch: n=${results?.size ?: 0}")
            results?.forEach { onScanResult(ScanSettings.CALLBACK_TYPE_ALL_MATCHES, it) }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e(TAG, "SCAN failed: $errorCode")
        }
    }

    private fun stopScanForce(reason: String) {
        mainHandler.post {
            try { scanner?.stopScan(scanCallback) } catch (t: Throwable) {
                Log.w(TAG, "stopScanForce($reason): stopScan threw: ${t.message}")
            }
            isScanning = false
            scanResultHandler = null
            Log.d(TAG, "SCAN stopped ($reason)")
        }
    }

    data class Candidate(val device: BluetoothDevice, val name: String?, val address: String, val rssi: Int)

    suspend fun scan(timeoutMs: Long): List<Candidate> =
        suspendCancellableCoroutine { cont ->
            if (adapter == null || !adapter.isEnabled) {
                emitState("BT not available")
                cont.resume(emptyList())
                return@suspendCancellableCoroutine
            }
            if (!hasBtScan()) {
                emitState("Scan aborted: permission")
                cont.resume(emptyList())
                return@suspendCancellableCoroutine
            }

            val candidates = mutableMapOf<String, Candidate>()

            if (isScanning) {
                mainHandler.post {
                    safeBt("stopScan(prev)", Unit) { scanner?.stopScan(scanCallback); Unit }
                    isScanning = false
                }
            }

            scanResultHandler = { r ->
                val device = r.device
                val addr = device.address
                val name = device.name ?: r.scanRecord?.deviceName
                val rssi = r.rssi
                val prev = candidates[addr]
                if (prev == null || rssi > prev.rssi) {
                    candidates[addr] = Candidate(device, name, addr, rssi)
                }
            }

            val settings = ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                .setMatchMode(ScanSettings.MATCH_MODE_AGGRESSIVE)
                .setNumOfMatches(ScanSettings.MATCH_NUM_MAX_ADVERTISEMENT)
                .setReportDelay(0L)
                .build()

            mainHandler.post {
                val started = safeBt("startScan", false) {
                    scanner?.startScan(null, settings, scanCallback)
                    true
                }
                if (!started) {
                    emitState("Scan start error")
                    if (cont.isActive) cont.resume(emptyList())
                    return@post
                }
                isScanning = true
                emitState("Scanning...")
            }

            mainHandler.postDelayed({
                mainHandler.post {
                    safeBt("stopScan(timeout)", Unit) { scanner?.stopScan(scanCallback); Unit }
                    isScanning = false
                    val list = candidates.values.sortedByDescending { it.rssi }
                    scanResultHandler = null
                    cont.resume(list.toList())
                }
            }, timeoutMs)

            cont.invokeOnCancellation {
                mainHandler.post {
                    safeBt("stopScan(cancel)", Unit) { scanner?.stopScan(scanCallback); Unit }
                    isScanning = false
                    scanResultHandler = null
                }
            }
        }

    // ---------- Connection / GATT state ----------
    private var currentMtu: Int = 23
    private var isReadyToWrite: Boolean = false
    private var probeMode = false
    private var probeCompleted = false
    private var servicesDiscoveryStarted = false
    private var mtuRequestInFlight = false
    private var connectionContinuation: kotlin.coroutines.Continuation<Boolean>? = null
    private var awaitingCccd = false
    private var cccdEnabledCallback: (() -> Unit)? = null
    private var cccdFallbackToken: Long = 0L

    /** Gibt true zurück wenn BLE verbunden und schreibbereit (CCCD aktiv). */
    fun isConnected(): Boolean = isReadyToWrite && gatt != null

    suspend fun connect(address: String, timeoutMs: Long): Boolean =
        suspendCancellableCoroutine { cont ->
            if (adapter == null || !adapter.isEnabled) {
                emitState("BT not available")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }
            if (!hasBtConnect()) {
                emitState("Connect aborted: permission")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }

            cleanupToken = 0L
            val attemptId = System.nanoTime()
            connectAttemptId = attemptId

            stopScanForce("connect()")

            val old = gatt
            try { old?.disconnect() } catch (_: Throwable) {}
            try { old?.close() } catch (_: Throwable) {}
            gatt = null
            rxChar = null
            txChar = null
            isReadyToWrite = false

            cancelBurst("new connect()")

            probeMode = true
            probeCompleted = false
            servicesDiscoveryStarted = false
            mtuRequestInFlight = false

            connectionContinuation = cont

            val device = try { adapter.getRemoteDevice(address) } catch (_: IllegalArgumentException) {
                emitState("Invalid address")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }

            var myGatt: BluetoothGatt? = null
            val ok = safeBt("connectGatt", false) {
                myGatt = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
                } else {
                    device.connectGatt(context, false, gattCallback)
                }
                gatt = myGatt
                true
            }
            if (!ok || myGatt == null) {
                probeMode = false
                connectionContinuation = null
                cont.resume(false)
                return@suspendCancellableCoroutine
            }

            mainHandler.postDelayed({
                if (connectAttemptId != attemptId) return@postDelayed
                if (gatt !== myGatt) return@postDelayed
                if (cont.isActive) {
                    Log.w(TAG, "connect timeout for $address")
                    try { myGatt?.disconnect() } catch (_: Throwable) {}
                    try { myGatt?.close() } catch (_: Throwable) {}
                    if (gatt === myGatt) {
                        gatt = null
                        rxChar = null
                        txChar = null
                        isReadyToWrite = false
                    }
                    probeMode = false
                    cont.resume(false)
                }
            }, timeoutMs)

            cont.invokeOnCancellation {
                if (connectAttemptId != attemptId) return@invokeOnCancellation
                if (gatt === myGatt) {
                    try { myGatt?.disconnect() } catch (_: Throwable) {}
                    try { myGatt?.close() } catch (_: Throwable) {}
                    gatt = null
                    rxChar = null
                    txChar = null
                    isReadyToWrite = false
                }
                probeMode = false
            }
        }

    suspend fun disconnect() {
        cancelBurst("disconnect()")
        val g0 = gatt
        safeBt("disconnect()", Unit) { g0?.disconnect(); Unit }
        safeBt("close()", Unit) { g0?.close(); Unit }
        if (gatt === g0) {
            gatt = null
            rxChar = null
            txChar = null
            isReadyToWrite = false
        }
        emitState("Disconnected")
    }

    // ---------- RX processing ----------
    private val rxBuffer = ArrayList<Byte>()

    @Volatile private var lastOkAtMs: Long = 0L
    @Volatile private var lastLine: String? = null
    @Volatile private var lastLineAtMs: Long = 0L

    private fun appendAndProcess(data: ByteArray) {
        for (b in data) rxBuffer.add(b)
        var found = true
        while (found) {
            found = false
            for (i in rxBuffer.indices) {
                if (rxBuffer[i] == '\n'.code.toByte()) {
                    val lineBytes = ByteArray(i + 1)
                    for (j in 0..i) lineBytes[j] = rxBuffer[j]
                    for (j in 0..i) rxBuffer.removeAt(0)
                    val decoded = String(lineBytes, Charset.forName("ISO-8859-1"))
                    val s = decoded.trimEnd('\r', '\n')

                    Log.d(TAG, "Line recv: $s")

                    lastLine = s
                    lastLineAtMs = nowMs()
                    if (s == "OK" || s.startsWith("ACK")) {
                        lastOkAtMs = nowMs()
                        Log.d(TAG, "appendAndProcess: OK observed -> lastOkAtMs=$lastOkAtMs")
                    }

                    emitLine(s)
                    found = true
                    break
                }
            }
        }
    }

    // ---------- BURST WRITER ----------
    private val BURST_CHUNK_SIZE = 20

    // With NO_RESPONSE we must pace ourselves; this is the "assume sent" step time.
    private val PER_CHUNK_ASSUME_MS = 160L
    private val MAX_ASSUME_PER_BURST = 40

    // Cooldown after each chunk to avoid BUSY(201)
    private val COOLDOWN_AFTER_ASSUME_MS = 90L
    private var cooldownUntilMs: Long = 0L

    private fun nowMs(): Long = SystemClock.uptimeMillis()

    // API33 statuscodes helper
    private fun statusSuccessCode(): Int {
        return try {
            val cls = Class.forName("android.bluetooth.BluetoothStatusCodes")
            cls.getField("SUCCESS").getInt(null)
        } catch (_: Throwable) { 0 }
    }

    private fun statusBusyCode(): Int {
        return try {
            val cls = Class.forName("android.bluetooth.BluetoothStatusCodes")
            cls.getField("ERROR_GATT_WRITE_REQUEST_BUSY").getInt(null)
        } catch (_: Throwable) { 201 }
    }

    private var burstToken: Long = 0L
    private var burstCont: kotlin.coroutines.Continuation<Boolean>? = null
    private var burstChunks: List<ByteArray> = emptyList()
    private var burstIndex: Int = 0
    private var burstInFlight: Boolean = false
    private var burstWaitingCallback: Boolean = false
    private var burstAssumeCount: Int = 0
    private var retryDelayMs: Long = 80L
    private var burstLastSentAtMs: Long = 0L

    // global write gate (prevents BUSY hammering)
    @Volatile private var writeInFlight: Boolean = false
    private var writeInFlightToken: Long = 0L
    private var writeInFlightIdx: Int = -1
    private var writeInFlightSeq: Long = 0L

    // Burst option: force WITH_RESPONSE for this burst (used for packetOTA/beginOTA/endOTA)
    private var burstForceWithResponse: Boolean = false

    private fun setWriteInFlight(token: Long, idx: Int, why: String) {
        writeInFlight = true
        writeInFlightToken = token
        writeInFlightIdx = idx
        Log.d(TAG, "writeInFlight=TRUE token=$token idx=$idx why=$why")
    }

    private fun clearWriteInFlight(why: String) {
        if (!writeInFlight) return
        Log.d(TAG, "writeInFlight=FALSE token=$writeInFlightToken idx=$writeInFlightIdx why=$why")
        writeInFlight = false
        writeInFlightToken = 0L
        writeInFlightIdx = -1
        writeInFlightSeq = 0L
    }

    private fun gateBusyWrite(token: Long, idx: Int, why: String, delayMs: Long = 140L) {
        setWriteInFlight(token, idx, "BUSY gate: $why")
        val seq = System.nanoTime()
        writeInFlightSeq = seq
        mainHandler.postDelayed({
            if (writeInFlightSeq != seq) return@postDelayed
            if (writeInFlight && writeInFlightToken == token && writeInFlightIdx == idx) {
                clearWriteInFlight("BUSY gate release: $why")
            }
        }, delayMs)
    }

    private fun cancelBurst(reason: String) {
        try {
            burstInFlight = false
            burstWaitingCallback = false
            burstChunks = emptyList()
            burstIndex = 0
            burstToken = 0L
            burstAssumeCount = 0
            retryDelayMs = 80L
            // clear any forced response flag on cancel
            burstForceWithResponse = false
            clearWriteInFlight("cancelBurst:$reason")
            burstCont?.let {
                Log.w(TAG, "cancelBurst($reason): resuming false")
                it.resume(false)
            }
        } catch (_: Throwable) {
        } finally {
            burstCont = null
        }
    }

    private fun scheduleAssumeSent(localToken: Long, idxAtSend: Int) {
        mainHandler.postDelayed({
            if (burstToken != localToken) return@postDelayed
            if (!burstInFlight) return@postDelayed
            if (!burstWaitingCallback) return@postDelayed
            if (burstIndex != idxAtSend) return@postDelayed

            burstAssumeCount++
            if (burstAssumeCount > MAX_ASSUME_PER_BURST) {
                Log.e(TAG, "Too many assume-sent in one burst -> failing")
                burstWaitingCallback = false
                burstInFlight = false
                clearWriteInFlight("assume-too-many")
                burstCont?.let { it.resume(false) }
                burstCont = null
                return@postDelayed
            }

            // accept this chunk as "sent"
            cooldownUntilMs = nowMs() + COOLDOWN_AFTER_ASSUME_MS
            burstWaitingCallback = false
            clearWriteInFlight("assume-sent")
            burstIndex += 1
            mainHandler.post { burstWriteNext(localToken) }
        }, PER_CHUNK_ASSUME_MS)
    }

    private fun burstWriteNext(localToken: Long) {
        if (burstToken != localToken) return
        if (!burstInFlight) return

        val g = gatt
        val rx = rxChar
        if (g == null || rx == null) {
            Log.e(TAG, "burstWriteNext: not connected")
            burstInFlight = false
            burstWaitingCallback = false
            burstCont?.let { it.resume(false) }
            burstCont = null
            return
        }
        if (!hasBtConnect()) {
            emitState("Write aborted: permission")
            burstInFlight = false
            burstWaitingCallback = false
            burstCont?.let { it.resume(false) }
            burstCont = null
            return
        }
        if (!isReadyToWrite) {
            mainHandler.postDelayed({ burstWriteNext(localToken) }, 60L)
            return
        }

        val now = nowMs()
        if (now < cooldownUntilMs) {
            mainHandler.postDelayed({ burstWriteNext(localToken) }, (cooldownUntilMs - now) + 10L)
            return
        }

        if (burstIndex >= burstChunks.size) {
            Log.d(TAG, "burst done (chunks=${burstChunks.size})")
            burstInFlight = false
            burstWaitingCallback = false
            burstAssumeCount = 0
            retryDelayMs = 80L
            // clear forced response flag after burst completes
            burstForceWithResponse = false
            burstCont?.let { it.resume(true) }
            burstCont = null
            return
        }

        if (burstWaitingCallback) {
            mainHandler.postDelayed({ burstWriteNext(localToken) }, 40L)
            return
        }

        // gate if stack is still "busy/inflight"
        if (writeInFlight) {
            mainHandler.postDelayed({ burstWriteNext(localToken) }, 60L)
            return
        }

        val chunk = burstChunks[burstIndex]
        val props = rx.properties
        val hasWriteNoRespProp = (props and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0
        val chosenWriteType = if (burstForceWithResponse) {
            // force WITH_RESPONSE for bursts that require ACK (e.g., packetOTA)
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        } else if (FORCE_WRITE_NO_RESPONSE) {
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        } else if (hasWriteNoRespProp) {
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        } else {
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        }

        // Final write type to use for the actual API call: ensure bursts that requested WITH_RESPONSE
        // always use WRITE_TYPE_DEFAULT regardless of global FORCE_WRITE_NO_RESPONSE.
        val finalWriteType = if (burstForceWithResponse) BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT else chosenWriteType

        // Minimal P0 debug: log actual chosen write type and properties for verification
        Log.d("BLE", "burstWriteNext: len=${chunk.size} type=${finalWriteType} props=${rx.properties} forceWithResp=${burstForceWithResponse}")

        Log.d(
            TAG,
            "burstWriteNext: sending chunk ${burstIndex + 1}/${burstChunks.size} len=${chunk.size} " +
                    "props=0x${props.toString(16)} propNoResp=$hasWriteNoRespProp forcedNoResp=$FORCE_WRITE_NO_RESPONSE chosen=$chosenWriteType"
        )

        // send
        val accepted: Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val status = safeBt("writeCharacteristic(API33)", -1) {
                @Suppress("DEPRECATION")
                // use finalWriteType to guarantee WITH_RESPONSE when requested
                g.writeCharacteristic(rx, chunk, finalWriteType)
            } as Int
            if (status == statusSuccessCode() || status == BluetoothGatt.GATT_SUCCESS) {
                true
            } else if (status == statusBusyCode()) {
                Log.w(TAG, "writeCharacteristic(API33) BUSY(status=$status) idx=$burstIndex -> gate & retry")
                gateBusyWrite(localToken, burstIndex, "burst BUSY(201)", 160L)
                mainHandler.postDelayed({ burstWriteNext(localToken) }, 80L)
                return
            } else {
                Log.e(TAG, "writeCharacteristic(API33) fatal status=$status")
                cancelBurst("writeCharacteristic fatal status=$status")
                safeBt("disconnect(write_fail)", Unit) { g.disconnect(); Unit }
                return
            }
        } else {
            // ensure legacy path also uses finalWriteType when WITH_RESPONSE is requested
            rx.writeType = finalWriteType
            rx.value = chunk
            val ok = safeBt("writeCharacteristic(legacy)", false) { g.writeCharacteristic(rx) } as Boolean
            if (!ok) {
                retryDelayMs = (retryDelayMs + 60L).coerceAtMost(600L)
                Log.w(TAG, "writeCharacteristic(legacy) false -> retry in ${retryDelayMs}ms")
                mainHandler.postDelayed({ burstWriteNext(localToken) }, retryDelayMs)
                return
            }
            true
        }

        if (!accepted) {
            cancelBurst("not accepted")
            return
        }

        // accepted -> in-flight + watchdog
        retryDelayMs = 80L
        burstWaitingCallback = true
        val idxAtSend = burstIndex
        burstLastSentAtMs = nowMs()
        setWriteInFlight(localToken, idxAtSend, "burst accepted")

        // With NO_RESPONSE we must advance via assume-sent pacing always.
        if (chosenWriteType == BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) {
            scheduleAssumeSent(localToken, idxAtSend)
        } else {
            // if someone disables FORCE_WRITE_NO_RESPONSE, we still protect against missing callback
            mainHandler.postDelayed({
                if (burstToken != localToken) return@postDelayed
                if (!burstInFlight) return@postDelayed
                if (!burstWaitingCallback) return@postDelayed
                if (burstIndex != idxAtSend) return@postDelayed

                // If we observed a line (OK/ERROR/ACK) after we sent the chunk, accept or fail accordingly
                if (lastLineAtMs >= burstLastSentAtMs && lastLine != null) {
                    val ll = lastLine!!
                    if (ll == "OK" || ll.startsWith("ACK")) {
                        Log.w(TAG, "WITH_RESPONSE watchdog: OK/ACK observed -> accept chunk ${idxAtSend + 1}/${burstChunks.size}")
                        burstWaitingCallback = false
                        clearWriteInFlight("with_response:line_ok_observed")
                        burstIndex += 1
                        mainHandler.post { burstWriteNext(localToken) }
                        return@postDelayed
                    } else if (ll == "ERROR") {
                        Log.e(TAG, "WITH_RESPONSE watchdog: ERROR observed -> fail burst ${idxAtSend + 1}/${burstChunks.size}")
                        burstWaitingCallback = false
                        burstInFlight = false
                        clearWriteInFlight("with_response:line_error_observed")
                        burstCont?.let { it.resume(false) }
                        burstCont = null
                        // Do NOT forcibly disconnect here; let higher-level protocol handle abort/cleanup
                        return@postDelayed
                    }
                }

                Log.e(TAG, "WITH_RESPONSE watchdog timeout idx=${idxAtSend + 1}/${burstChunks.size} -> abort")
                burstWaitingCallback = false
                burstInFlight = false
                clearWriteInFlight("with_response:timeout")
                burstCont?.let { it.resume(false) }
                burstCont = null
                safeBt("disconnect(write_timeout)", Unit) { g.disconnect(); Unit }
            }, 2000L)
        }
    }

    private suspend fun burstWrite(bytes: ByteArray, forceWithResponse: Boolean = false): Boolean =
        suspendCancellableCoroutine { cont ->
            val g = gatt
            val rx = rxChar
            if (g == null || rx == null) {
                Log.e(TAG, "burstWrite: not connected")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }
            if (!hasBtConnect()) {
                Log.e(TAG, "burstWrite: missing BLUETOOTH_CONNECT")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }

            if (burstInFlight || burstCont != null) {
                Log.w(TAG, "burstWrite: busy (burstInFlight=$burstInFlight)")
                cont.resume(false)
                return@suspendCancellableCoroutine
            }

            val chunks = ArrayList<ByteArray>()
            var i = 0
            while (i < bytes.size) {
                val end = minOf(i + BURST_CHUNK_SIZE, bytes.size)
                chunks.add(bytes.copyOfRange(i, end))
                i = end
            }

            val localToken = System.nanoTime()
            burstToken = localToken
            // store caller preference for WITH_RESPONSE for this burst
            burstForceWithResponse = forceWithResponse
            burstChunks = chunks
            burstIndex = 0
            burstInFlight = true
            burstWaitingCallback = false
            burstAssumeCount = 0
            retryDelayMs = 80L
            burstCont = cont

            mainHandler.post { burstWriteNext(localToken) }

            cont.invokeOnCancellation {
                if (burstToken == localToken) cancelBurst("cancelled")
            }
        }

    // ---------- writing API ----------
    suspend fun writeLine(line: String) {
        stopScanForce("pre-write")
        val g = gatt
        val rx = rxChar
        if (g == null || rx == null) {
            Log.e(TAG, "writeLine: not connected")
            emitState("Write failed: not connected")
            return
        }
        if (!hasBtConnect()) {
            emitState("Write aborted: permission")
            return
        }

        val out = if (line.endsWith("\n") || line.endsWith("\r\n")) {
            line.toByteArray(Charset.forName("ISO-8859-1"))
        } else {
            (line + "\r\n").toByteArray(Charset.forName("ISO-8859-1"))
        }

        Log.d(TAG, "TX -> $line (burst 20-byte)")
        emitState("TX: ${if (line.startsWith("response=")) "response=..." else line}")
        burstWrite(out)
    }

    /**
     * writeOnce:
     * - For small ASCII commands we WAIT FOR NOTIFY ack (hex+OK or OK/ACK).
     * - We DO NOT depend on onCharacteristicWrite (device may never deliver it on time, can end in 133).
     */
    suspend fun writeOnce(bytes: ByteArray, timeoutMs: Long = 6000L): Boolean {
        val g = gatt
        val rx = rxChar
        if (g == null || rx == null) {
            Log.e(TAG, "writeOnce: not connected")
            return false
        }
        if (!hasBtConnect()) {
            Log.e(TAG, "writeOnce: missing BLUETOOTH_CONNECT")
            return false
        }

        val asText = try { String(bytes, Charset.forName("ISO-8859-1")).trim() } catch (_: Throwable) { "" }
        // consider CR/LF variants by trimming
        val asTextNoEol = asText.trimEnd('\r', '\n')
        val isSmallAsciiCmd = bytes.size <= BURST_CHUNK_SIZE && (
                asTextNoEol.startsWith("challenge", ignoreCase = true) ||
                        asTextNoEol.startsWith("gethardware", ignoreCase = true) ||
                        asTextNoEol.startsWith("getbuild", ignoreCase = true) ||
                        asTextNoEol.equals("beginOTA", ignoreCase = true) ||
                        asTextNoEol.equals("endOTA", ignoreCase = true) ||
                        asTextNoEol.equals("abortOTA", ignoreCase = true)
                )

        if (!isSmallAsciiCmd) {
            // default: chunked burst
            val asText = try { String(bytes, Charset.forName("ISO-8859-1")).trim() } catch (_: Throwable) { "" }
            val forceWithResp = asText.startsWith("packetOTA=", ignoreCase = true) ||
                    asText.equals("beginOTA", ignoreCase = true) ||
                    asText.equals("endOTA", ignoreCase = true)
            return burstWrite(bytes, forceWithResp)
        }

        Log.d(TAG, "writeOnce: small ASCII cmd -> notify-ACK wait: '$asTextNoEol'")
        Log.d(TAG, "TX -> $asText")

        // choose write type
        val props = rx.properties
        val hasWriteNoRespProp = (props and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0
        val chosenWriteType = if (FORCE_WRITE_NO_RESPONSE) {
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        } else if (hasWriteNoRespProp) {
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        } else {
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        }
        Log.d(TAG, "writeOnce: props=0x${props.toString(16)} propNoResp=$hasWriteNoRespProp forcedNoResp=$FORCE_WRITE_NO_RESPONSE chosen=$chosenWriteType")

        // register listener BEFORE sending (device answers very fast)
        var gotHex: String? = null
        var gotOk = false

        val cb: (String) -> Unit = { line ->
            val s = line.trim()
            if (asTextNoEol.startsWith("challenge", ignoreCase = true)) {
                if (isHex64(s)) gotHex = s
                if (s == "OK") gotOk = true
            } else if (asTextNoEol.equals("beginOTA", ignoreCase = true) || asTextNoEol.equals("endOTA", ignoreCase = true) || asTextNoEol.equals("abortOTA", ignoreCase = true)) {
                // for OTA control commands, accept exactly OK or ERROR as first response
                if (s == "OK" || s == "ERROR") gotOk = true
            } else {
                if (s == "OK" || s.startsWith("ACK")) gotOk = true
            }
        }
        addOnLineListener(cb)

        try {
            // gate a little if stack currently busy
            if (writeInFlight) {
                val start = nowMs()
                while (writeInFlight && (nowMs() - start) < 1200L) delay(20L)
                if (writeInFlight) clearWriteInFlight("writeOnce pre-gate timeout")
            }

            // write with BUSY backoff
            var attempts = 0
            while (attempts < 8) {
                attempts += 1
                val okSend: Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    val status = safeBt("writeCharacteristic(API33)", -1) {
                        @Suppress("DEPRECATION")
                        g.writeCharacteristic(rx, bytes, chosenWriteType)
                    } as Int
                    if (status == statusSuccessCode() || status == BluetoothGatt.GATT_SUCCESS) {
                        true
                    } else if (status == statusBusyCode()) {
                        Log.w(TAG, "writeOnce BUSY(201) attempt=$attempts -> delay")
                        gateBusyWrite(System.nanoTime(), 0, "writeOnce BUSY(201)", 140L)
                        delay(80L)
                        false
                    } else {
                        Log.e(TAG, "writeOnce fatal status=$status")
                        return false
                    }
                } else {
                    rx.writeType = chosenWriteType
                    rx.value = bytes
                    val ok = safeBt("writeCharacteristic(legacy)", false) { g.writeCharacteristic(rx) } as Boolean
                    if (!ok) {
                        delay((80L * attempts).coerceAtMost(600L))
                    }
                    ok
                }

                if (okSend) break
            }

            // short gate after accepted write (prevents immediate BUSY on next command)
            setWriteInFlight(System.nanoTime(), 0, "writeOnce accepted smallCmd")
            mainHandler.postDelayed({ clearWriteInFlight("writeOnce smallCmd gate release") }, 200L)

            // wait for NOTIFY ACK
            val startWait = nowMs()
            while ((nowMs() - startWait) < timeoutMs) {
                if (asTextNoEol.startsWith("challenge", ignoreCase = true)) {
                    if (gotHex != null && gotOk) return true
                } else {
                    if (gotOk) return true
                }
                delay(20L)
            }

            Log.w(TAG, "writeOnce: notify-ACK timeout for '$asTextNoEol' (gotHex=${gotHex != null}, gotOk=$gotOk)")
            return false
        } finally {
            try { onLineListeners.remove(cb) } catch (_: Throwable) {}
        }
    }

    private fun isHex64(s: String): Boolean {
        val t = s.trim()
        if (t.length != 64) return false
        return t.all { it in '0'..'9' || it in 'a'..'f' || it in 'A'..'F' }
    }

    // ---------- UNLOCK flow (single connection) ----------
    suspend fun unlockSingleConnection(address: String, connectTimeoutMs: Long = 15000L): Boolean {
        Log.d(TAG, "### UNLOCK_SINGLE_CONNECTION START ###")
        try {
            try { disconnect() } catch (_: Throwable) {}
            delay(350)

            val okConnect = connect(address, connectTimeoutMs)
            if (!okConnect) {
                Log.w(TAG, "unlockSingleConnection: connect failed")
                return false
            }

            // 1) challenge (NOTIFY returns HEX + OK)
            var challengeHex: String? = null
            var gotOk1 = false
            val cb1: (String) -> Unit = { line ->
                val s = line.trim()
                if (isHex64(s)) challengeHex = s
                if (s == "OK") gotOk1 = true
            }
            addOnLineListener(cb1)

            val wroteCh = writeOnce("challenge\r\n".toByteArray(Charsets.ISO_8859_1), timeoutMs = 8000L)
            try { onLineListeners.remove(cb1) } catch (_: Throwable) {}

            if (!wroteCh || challengeHex == null || !gotOk1) {
                Log.w(TAG, "unlockSingleConnection: challenge writeOnce failed (wrote=$wroteCh hex=${challengeHex != null} ok=$gotOk1)")
                return false
            }

            val ch = challengeHex!!

            delay(250)

            // 2) sign
            val sig = CureCrypto.buildUnlockResponse(ch)

            // 3) response=... (device should reply OK via NOTIFY)
            var gotOk2 = false
            val cb2: (String) -> Unit = { line ->
                if (line.trim() == "OK") gotOk2 = true
            }
            addOnLineListener(cb2)

            val pkt = "response=$sig\r\n".toByteArray(Charsets.ISO_8859_1)
            val wroteResp = burstWrite(pkt) // large -> burst
            if (!wroteResp) {
                try { onLineListeners.remove(cb2) } catch (_: Throwable) {}
                Log.w(TAG, "unlockSingleConnection: response burstWrite failed")
                return false
            }

            // wait for OK
            val startWait = nowMs()
            while ((nowMs() - startWait) < 20000L) {
                if (gotOk2) break
                delay(20L)
            }
            try { onLineListeners.remove(cb2) } catch (_: Throwable) {}

            Log.d(TAG, "unlockSingleConnection: ok=$gotOk2")
            return gotOk2
        } finally {
            // keep connection as-is; caller decides
        }
    }

    // ---------- GATT callback ----------
    private val gattCallback = object : BluetoothGattCallback() {

        private fun isStale(g: BluetoothGatt): Boolean {
            val cur = this@BleUartClient.gatt
            if (cur == null) return false
            return g !== cur
        }

        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            super.onConnectionStateChange(g, status, newState)
            if (isStale(g)) return

            Log.d(TAG, "GATT state changed: status=$status newState=$newState")

            if (status != BluetoothGatt.GATT_SUCCESS) {
                emitState("GATT error: $status")
                connectionContinuation?.let { it.resume(false); connectionContinuation = null }
                probeMode = false
                return
            }

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                emitState("Connected")
                stopScanForce("connected")
                isReadyToWrite = false

                safeBt("requestConnPriority", Unit) { g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); Unit }

                if (!mtuRequestInFlight) {
                    mtuRequestInFlight = true
                    safeBt("requestMtu", Unit) { g.requestMtu(185); Unit }
                }

                mainHandler.postDelayed({
                    if (isStale(g)) return@postDelayed
                    if (!servicesDiscoveryStarted) {
                        servicesDiscoveryStarted = true
                        Log.d(TAG, "discoverServices(fallback-after-connect)")
                        safeBt("discoverServices(fallback-after-connect)", Unit) { g.discoverServices(); Unit }
                    }
                }, 350L)

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                emitState("Disconnected")
                cancelBurst("disconnected")
                connectionContinuation?.let { it.resume(false); connectionContinuation = null }
                probeMode = false
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            super.onMtuChanged(g, mtu, status)
            if (isStale(g)) return

            if (status == BluetoothGatt.GATT_SUCCESS) currentMtu = mtu
            Log.d(TAG, "MTU changed: mtu=$mtu status=$status currentMtu=$currentMtu")
            mtuRequestInFlight = false

            if (hasBtConnect() && !servicesDiscoveryStarted) {
                servicesDiscoveryStarted = true
                Log.d(TAG, "discoverServices(after-mtu)")
                safeBt("discoverServices(after-mtu)", Unit) { g.discoverServices(); Unit }
            }

            mainHandler.postDelayed({
                if (isStale(g)) return@postDelayed
                if (!probeCompleted) {
                    Log.w(TAG, "services still not discovered -> retry discoverServices")
                    safeBt("discoverServices(retry-after-connect)", Unit) { g.discoverServices(); Unit }
                }
            }, 1000L)
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            super.onServicesDiscovered(g, status)
            if (isStale(g)) return

            Log.d(TAG, "Services discovered: status=$status")

            if (status != BluetoothGatt.GATT_SUCCESS) {
                emitState("Service discovery failed")
                connectionContinuation?.let { it.resume(false); connectionContinuation = null }
                probeMode = false
                return
            }

            if (!hasBtConnect()) {
                emitState("Permission missing: BLUETOOTH_CONNECT")
                connectionContinuation?.let { it.resume(false); connectionContinuation = null }
                probeMode = false
                return
            }

            if (!probeMode || probeCompleted) return
            probeCompleted = true

            val nus = g.getService(UART_SERVICE_UUID)
            val rx = nus?.getCharacteristic(UART_RX_UUID)
            val tx = nus?.getCharacteristic(UART_TX_UUID)

            if (nus == null || rx == null || tx == null) {
                emitState("Probe failed: not CureBase device")
                safeBt("disconnect(probeFail)", Unit) { g.disconnect(); Unit }
                safeBt("close(probeFail)", Unit) { g.close(); Unit }
                connectionContinuation?.let { it.resume(false); connectionContinuation = null }
                probeMode = false
                return
            }

            rxChar = rx
            txChar = tx

            Log.d(TAG, "PROBE ok: NUS found RX=${rx.uuid} TX=${tx.uuid}")
            val props = rx.properties
            val write = (props and BluetoothGattCharacteristic.PROPERTY_WRITE) != 0
            val writeNr = (props and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0
            Log.d(TAG, "RX props=0x${props.toString(16)} WRITE=$write WRITE_NR=$writeNr")
            emitState("RX props=0x${props.toString(16)} WRITE=$write WRITE_NR=$writeNr")

            val okNotify = safeBt("setCharacteristicNotification", false) {
                g.setCharacteristicNotification(txChar, true)
            }
            Log.d(TAG, "setCharacteristicNotification returned $okNotify")

            val cccUuid = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
            val descriptor = txChar?.getDescriptor(cccUuid)

            if (descriptor != null) {
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE

                val wrote: Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    val r = safeBt("writeDescriptor(API33)", -1) {
                        @Suppress("DEPRECATION")
                        g.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                    } as Int
                    Log.d(TAG, "writeDescriptor(API33) status=$r")
                    (r == statusSuccessCode() || r == BluetoothGatt.GATT_SUCCESS)
                } else {
                    safeBt("writeDescriptor(CCCD)", false) { g.writeDescriptor(descriptor) } as Boolean
                }

                if (wrote) {
                    awaitingCccd = true
                    cccdEnabledCallback = {
                        Log.d(TAG, "NOTIFY enabled TX -> READY")
                        stopScanForce("READY")
                        safeBt("requestConnPriority", Unit) { g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); Unit }
                        emitState("Connected and ready")
                        isReadyToWrite = true
                        connectionContinuation?.let { it.resume(true); connectionContinuation = null }
                        probeMode = false
                    }

                    cccdFallbackToken = System.nanoTime()
                    val token = cccdFallbackToken
                    mainHandler.postDelayed({
                        if (isStale(g)) return@postDelayed
                        if (awaitingCccd && cccdFallbackToken == token) {
                            Log.w(TAG, "CCCD fallback: onDescriptorWrite not received -> proceeding READY")
                            awaitingCccd = false
                            cccdEnabledCallback?.invoke()
                            cccdEnabledCallback = null
                        }
                    }, 800L)

                } else {
                    Log.w(TAG, "writeDescriptor returned false; proceeding READY")
                    awaitingCccd = false
                    stopScanForce("READY")
                    safeBt("requestConnPriority", Unit) { g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); Unit }
                    emitState("Connected and ready")
                    isReadyToWrite = true
                    connectionContinuation?.let { it.resume(true); connectionContinuation = null }
                    probeMode = false
                }
            } else {
                Log.w(TAG, "CCC descriptor not found on TX; proceeding READY")
                awaitingCccd = false
                stopScanForce("READY")
                safeBt("requestConnPriority", Unit) { g.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); Unit }
                emitState("Connected and ready")
                isReadyToWrite = true
                connectionContinuation?.let { it.resume(true); connectionContinuation = null }
                probeMode = false
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            super.onDescriptorWrite(g, descriptor, status)
            if (isStale(g)) return

            Log.d(TAG, "onDescriptorWrite status=$status uuid=${descriptor.uuid}")
            cccdFallbackToken = 0L

            val cccUuid = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
            if (descriptor.uuid == cccUuid) {
                Log.d(TAG, "CCCD write status=$status -> isReadyToWrite=true")
                isReadyToWrite = true
            }

            if (awaitingCccd) {
                awaitingCccd = false
                cccdEnabledCallback?.invoke()
                cccdEnabledCallback = null
            }
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            super.onCharacteristicChanged(g, characteristic)
            if (isStale(g)) return
            if (characteristic.uuid == txChar?.uuid) {
                val data = characteristic.value ?: return
                appendAndProcess(data)
            }
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            super.onCharacteristicWrite(g, characteristic, status)
            if (isStale(g)) return

            // For this device, callbacks can be late/133. We log it, but we do not rely on it for success.
            Log.d(TAG, "onCharacteristicWrite: uuid=${characteristic.uuid} status=$status")

            // if we ever forced WITH_RESPONSE, we'd advance burst here; but in default mode we use assume-sent pacing.
            if (status != BluetoothGatt.GATT_SUCCESS) {
                // still clear gate to avoid permanent lock
                if (writeInFlight) clearWriteInFlight("onCharacteristicWrite status=$status")
                return
            }

            // success callback: clear gate (helpful if it arrives)
            if (writeInFlight) clearWriteInFlight("onCharacteristicWrite success")

            // If a burst used WITH_RESPONSE (burstForceWithResponse) OR FORCE_WRITE_NO_RESPONSE is disabled,
            // then advance the burst on the write callback.
            if (burstInFlight && burstWaitingCallback && (burstForceWithResponse || !FORCE_WRITE_NO_RESPONSE)) {
                burstWaitingCallback = false
                burstIndex += 1
                mainHandler.post { burstWriteNext(burstToken) }
            }
        }
    }
}
