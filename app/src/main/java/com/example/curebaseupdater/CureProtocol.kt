package com.example.curebaseupdater

import android.bluetooth.BluetoothGattCharacteristic
import android.util.Log
import kotlinx.coroutines.delay
import kotlinx.coroutines.withTimeoutOrNull
import java.util.ArrayDeque

private const val TAG = "CURE_UPDATER"

class CureProtocol(private val client: BleUartClient) {

    private val rxLines: ArrayDeque<String> = ArrayDeque()

    init {
        client.addOnLineListener { line ->
            Log.d(TAG, "RX <- $line")
            synchronized(rxLines) { rxLines.addLast(line) }
        }
        client.addOnStateListener { st ->
            Log.d(TAG, "STATE: $st")
        }
    }

    private fun clearRxLines() {
        synchronized(rxLines) { rxLines.clear() }
    }

    private suspend fun waitForLine(timeoutMs: Long): String? {
        val deadline = System.currentTimeMillis() + timeoutMs
        while (System.currentTimeMillis() < deadline) {
            val line: String? = synchronized(rxLines) {
                if (rxLines.isNotEmpty()) rxLines.removeFirst() else null
            }
            if (line != null) return line
            delay(30)
        }
        return null
    }

    private suspend fun sendAndWaitFor(
        predicate: (String) -> Boolean,
        send: suspend () -> Unit,
        timeoutMs: Long = 3000L,
        retries: Int = 1
    ): String? {
        repeat(retries + 1) { attempt ->
            clearRxLines()

            try {
                send()
            } catch (e: Exception) {
                Log.e(TAG, "send failed", e)
                return null
            }

            val hit: String? = withTimeoutOrNull<String?>(timeoutMs) {
                while (true) {
                    val line = waitForLine(500) ?: continue
                    if (predicate(line)) return@withTimeoutOrNull line
                }
                // IMPORTANT: makes lambda return String? instead of Unit (compiler requirement)
                null
            }

            if (hit != null) return hit
            Log.w(TAG, "Timeout waiting for response (attempt ${attempt + 1})")
        }
        return null
    }

    suspend fun requestChallenge(): String? {
        Log.d(TAG, "requestChallenge()")
        clearRxLines()

        // enqueueWrite(...) no longer exists; use the suspend writeLine API which appends CRLF
        client.writeLine("challenge")

        val deadline = System.currentTimeMillis() + 3000
        val lines = mutableListOf<String>()

        while (System.currentTimeMillis() < deadline) {
            val line = waitForLine(timeoutMs = 500) ?: continue
            lines += line
            if (line.equals("OK", ignoreCase = true)) continue

            val m = Regex("([0-9A-Fa-f]{64})").find(line)
            if (m != null) {
                val challenge = m.groupValues[1].uppercase()
                Log.d(TAG, "challengeHex=$challenge")
                return challenge
            }
        }

        Log.w(TAG, "requestChallenge: no valid challenge found, lines=${lines.joinToString("|")}")
        return null
    }

    suspend fun sendUnlockResponse(sigHex: String): Boolean {
        Log.d(TAG, "sendUnlockResponse(len=${sigHex.length})")

        val line = sendAndWaitFor(
            predicate = { it.equals("OK", ignoreCase = true) || it.contains("unlocked", ignoreCase = true) },
            send = {
                // small pause before sending response to allow remote to be ready
                kotlinx.coroutines.delay(200)
                // use existing suspend writeOnce API on the client
                val ok = client.writeOnce("response=$sigHex".toByteArray(Charsets.ISO_8859_1))
                if (!ok) throw Exception("writeOnce failed")
            },
            timeoutMs = 8000L,
            retries = 1
        )
        return line != null
    }

    private fun line(cmd: String): ByteArray = (cmd + "\r\n").toByteArray(Charsets.ISO_8859_1)

    suspend fun beginOta(): Boolean {
        Log.d(TAG, "beginOta")
        val lineResp = sendAndWaitFor(
            predicate = { it.equals("OK", ignoreCase = true) || it.equals("ERROR", ignoreCase = true) },
            send = {
                Log.d(TAG, "TX -> beginOTA\r\n")
                val ok = client.writeOnce(line("beginOTA"))
                if (!ok) throw Exception("writeOnce failed")
            },
            timeoutMs = 3000L,
            retries = 2
        )
        Log.d(TAG, "beginOta: firstResp=$lineResp")
        if (lineResp == null) Log.w(TAG, "beginOta: no OK/ERROR response")
        return lineResp != null
    }

    suspend fun sendOtaPacket(encoded: String): Boolean {
        Log.d(TAG, "sendOtaPacket len=${encoded.length}")
        // DEBUG: log prefix of encoded packet to help debug device ERROR responses
        Log.d(TAG, "DEBUG sendOtaPacket prefix='${encoded.take(64)}' len=${encoded.length}")
        val lineResp = sendAndWaitFor(
            predicate = { it.equals("OK", ignoreCase = true) || it.equals("ERROR", ignoreCase = true) },
            send = {
                val ok = client.writeOnce(line("packetOTA=$encoded"))
                if (!ok) throw Exception("writeOnce failed")
            },
            timeoutMs = 3000L,
            retries = 2
        )
        if (lineResp == null) {
            Log.w(TAG, "sendOtaPacket: no response")
            return false
        }
        return lineResp.equals("OK", ignoreCase = true)
    }

    suspend fun endOta(): Boolean {
        Log.d(TAG, "endOta")
        val lineResp = sendAndWaitFor(
            predicate = { it.equals("OK", ignoreCase = true) || it.equals("ERROR", ignoreCase = true) || it.contains("reboot", true) },
            send = {
                Log.d(TAG, "TX -> endOTA\r\n")
                val ok = client.writeOnce(line("endOTA"))
                if (!ok) throw Exception("writeOnce failed")
            },
            timeoutMs = 3000L,
            retries = 2
        )
        Log.d(TAG, "endOta: firstResp=$lineResp")
        if (lineResp == null) Log.w(TAG, "endOta: no OK/ERROR/reboot response")
        return lineResp != null
    }

    suspend fun abortOta(): Boolean {
        Log.d(TAG, "abortOta")
        val lineResp = sendAndWaitFor(
            predicate = { it.equals("OK", ignoreCase = true) || it.equals("ERROR", ignoreCase = true) },
            send = {
                Log.d(TAG, "TX -> abortOTA\r\n")
                val ok = client.writeOnce(line("abortOTA"))
                if (!ok) throw Exception("writeOnce failed")
            },
            timeoutMs = 3000L,
            retries = 2
        )
        Log.d(TAG, "abortOta: firstResp=$lineResp")
        if (lineResp == null) Log.w(TAG, "abortOta: no OK/ERROR response")
        return lineResp != null
    }

    // removed probeOtaKeypair stub — probing is implemented in OtaSender (requires Context)
}