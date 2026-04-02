package com.example.curebaseupdater

import android.content.Context
import android.net.Uri
import android.util.Log
import kotlinx.coroutines.*
import java.io.InputStream

private const val TAG = "CURE_UPDATER"

class OtaSender(private val protocol: CureProtocol, private val context: Context) {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    suspend fun sendFromAssets(assetPath: String, progress: (sent: Int, total: Int) -> Unit): Boolean {
        val am = context.assets
        val ist: InputStream
        try {
            ist = am.open(assetPath)
        } catch (e: Exception) {
            Log.e(TAG, "Asset open failed", e)
            return false
        }
        return sendStream(ist, progress)
    }

    suspend fun sendFromUri(uri: Uri, progress: (sent: Int, total: Int) -> Unit): Boolean {
        val ist: InputStream
        try {
            ist = context.contentResolver.openInputStream(uri) ?: return false
        } catch (e: Exception) {
            Log.e(TAG, "Uri open failed", e)
            return false
        }
        return sendStream(ist, progress)
    }

    private suspend fun sendStream(ist: InputStream, progress: (sent: Int, total: Int) -> Unit): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                // 1) Firmware bytes lesen (erstmal simpel – später können wir streamen)
                val fwRaw = ist.readBytes()
                Log.d(TAG, "Firmware raw size: ${fwRaw.size}")

                // 2) zlib compress (Firmware erwartet zlib wrapper)
                val fwZ = OtaCrypto.zlibCompress(fwRaw)
                Log.d(TAG, "Firmware zlib compressed size: ${fwZ.size}")

                // 3) AES-CTR encrypt als STREAM (Counter läuft über alle Updates weiter)
                val key = OtaCrypto.OTA_KEYS[OtaCrypto.otaKeyPairIndex]
                val iv = OtaCrypto.OTA_IVS[OtaCrypto.otaKeyPairIndex]
                Log.d(TAG, "Using OTA keypair index=${OtaCrypto.otaKeyPairIndex}")
                val cipher = OtaCrypto.aesCtrEncryptStream(key, iv)
                val fwEnc = cipher.doFinal(fwZ)
                Log.d(TAG, "Firmware encrypted size: ${fwEnc.size}")

                var sent = 0

                if (!protocol.beginOta()) {
                    Log.e(TAG, "beginOta failed")
                    return@withContext false
                }

                // Small pause to let the device enter OTA state fully (avoid race where first packet arrives too fast)
                kotlinx.coroutines.delay(80L)

                // Quick probe: send one small encrypted zlib-empty packet to verify keypair/crypto before sending full firmware
                try {
                    Log.d(TAG, "OTA probe: sending probe packet with current keypair index=${OtaCrypto.otaKeyPairIndex}")
                    val probeOk = sendProbePacketForCurrentKey()
                    if (!probeOk) {
                        Log.w(TAG, "OTA probe failed with current keypair=${OtaCrypto.otaKeyPairIndex}, running probeKeypairs()")
                        // abort current OTA session and run full probe
                        try { protocol.abortOta() } catch (_: Exception) {}
                        val found = probeKeypairs()
                        if (found == null) {
                            Log.w(TAG, "probeKeypairs found no valid keypair")
                            return@withContext false
                        }
                        Log.d(TAG, "probeKeypairs selected index=$found; restarting beginOta")
                        if (!protocol.beginOta()) {
                            Log.e(TAG, "beginOta failed after probe")
                            return@withContext false
                        }
                        kotlinx.coroutines.delay(80L)
                    } else {
                        Log.d(TAG, "OTA probe OK with index=${OtaCrypto.otaKeyPairIndex}")
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "OTA probe exception: ${e.message}")
                    try { protocol.abortOta() } catch (_: Exception) {}
                    val found = probeKeypairs()
                    if (found == null) {
                        Log.w(TAG, "probeKeypairs found no valid keypair (exception path)")
                        return@withContext false
                    }
                    Log.d(TAG, "probeKeypairs selected index=$found; restarting beginOta")
                    if (!protocol.beginOta()) {
                        Log.e(TAG, "beginOta failed after probe")
                        return@withContext false
                    }
                    kotlinx.coroutines.delay(80L)
                }

                val rawChunkSize = 512
                var offset = 0
                val total = fwEnc.size

                while (offset < fwEnc.size) {
                    val end = (offset + rawChunkSize).coerceAtMost(fwEnc.size)
                    val chunk = fwEnc.copyOfRange(offset, end)

                    val encoded = OtaEncoder.encode(chunk)

                    // send "packetOTA=<encoded>" und erwarte OK
                    val ok = protocol.sendOtaPacket(encoded)
                    if (!ok) throw IllegalStateException("packetOTA failed at offset=$offset")

                    sent += chunk.size
                    progress(sent, total)
                    delay(15)
                    offset = end
                }

                val endOk = protocol.endOta()
                if (!endOk) {
                    Log.e(TAG, "endOta failed")
                    return@withContext false
                }
                return@withContext true
            } catch (e: Exception) {
                Log.e(TAG, "OTA send error", e)
                return@withContext false
            } finally {
                try { ist.close() } catch (_: Exception) {}
            }
        }
    }

    // helper used for probing keypairs: send a single packetOTA containing the encrypted zlib-empty-stream
    suspend fun sendProbePacketForCurrentKey(): Boolean {
        // zlib empty stream (minimal): 78 9C 03 00 00 00 00 01
        val zlibEmpty = byteArrayOf(0x78.toByte(), 0x9C.toByte(), 0x03.toByte(), 0x00.toByte(), 0x00.toByte(), 0x00.toByte(), 0x00.toByte(), 0x01.toByte())
        val key = OtaCrypto.OTA_KEYS[OtaCrypto.otaKeyPairIndex]
        val iv = OtaCrypto.OTA_IVS[OtaCrypto.otaKeyPairIndex]
        Log.d(TAG, "probe: using keypair index=${OtaCrypto.otaKeyPairIndex}")
        val cipher = OtaCrypto.aesCtrEncryptStream(key, iv)
        val enc = cipher.doFinal(zlibEmpty)
        val encoded = OtaEncoder.encode(enc)
        Log.d(TAG, "probe: packetOTA encoded len=${encoded.length}")
        val ok = protocol.sendOtaPacket(encoded)
        Log.d(TAG, "probe: packetOTA result=$ok")
        return ok
    }

    // Probe OTA keypairs 0..(maxPairs-1). Returns found index or null.
    suspend fun probeKeypairs(maxPairs: Int = 16): Int? {
        for (i in 0 until maxPairs) {
            Log.d(TAG, "probeKeypairs: trying index=$i")
            OtaCrypto.otaKeyPairIndex = i
            try {
                // start OTA mode (line-based)
                val began = protocol.beginOta()
                if (!began) {
                    Log.w(TAG, "probeKeypairs: beginOta failed for index=$i")
                    // try next
                    continue
                }

                // send single encrypted empty zlib packet
                val ok = sendProbePacketForCurrentKey()

                // Always abort OTA afterwards to avoid leaving device in OTA state
                try {
                    val aborted = protocol.abortOta()
                    Log.d(TAG, "probeKeypairs: abortOta result=$aborted for index=$i")
                } catch (e: Exception) {
                    Log.w(TAG, "probeKeypairs: abortOta exception for index=$i: ${e.message}")
                }

                if (ok) {
                    Log.d(TAG, "probeKeypairs: success index=$i")
                    // keep otaKeyPairIndex set to i
                    return i
                } else {
                    Log.d(TAG, "probeKeypairs: packet failed index=$i -> trying next")
                    delay(120)
                }
            } catch (e: Exception) {
                Log.w(TAG, "probeKeypairs: exception for index=$i: ${e.message}")
                try { protocol.abortOta() } catch (_: Exception) {}
                delay(120)
            }
        }
        Log.w(TAG, "probeKeypairs: no valid keypair found")
        return null
    }
}
