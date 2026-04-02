package com.example.curebaseupdater

import java.nio.charset.Charset

// Minimal extension to provide unlockSingleConnection if not present in BleUartClient.
// This is a conservative stub: connect, send a simple "unlock" payload via writeOnce, then disconnect.
// Adjust to match your real unlock protocol as needed.

suspend fun BleUartClient.unlockSingleConnection(address: String, connectTimeoutMs: Long = 15000L): Boolean {
    try {
        val okConnect = this.connect(address, connectTimeoutMs)
        if (!okConnect) return false

        val payload = "unlock".toByteArray(Charset.forName("ISO-8859-1"))
        val okWrite = this.writeOnce(payload, timeoutMs = 6000L)

        // best-effort disconnect after unlock attempt
        try { this.disconnect() } catch (_: Throwable) {}

        return okWrite
    } catch (_: Throwable) {
        try { this.disconnect() } catch (_: Throwable) {}
        return false
    }
}
