package com.example.curebaseupdater

object OtaEncoder {
    // Encodes bytes into yEnc-like variant described
    fun encode(raw: ByteArray): String {
        val out = ByteArray(raw.size * 2) // worst case every byte becomes two
        var idx = 0
        for (b in raw) {
            val unsigned = b.toInt() and 0xFF
            if (unsigned == 0x00 || unsigned == 0x0A || unsigned == 0x0D || unsigned == '='.code) {
                out[idx++] = '='.code.toByte()
                val v = (unsigned + 42 + 64) and 0xFF
                out[idx++] = v.toByte()
            } else {
                val v = (unsigned + 42) and 0xFF
                out[idx++] = v.toByte()
            }
        }
        return String(out, 0, idx, Charsets.ISO_8859_1)
    }
}

