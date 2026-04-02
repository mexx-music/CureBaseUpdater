package com.example.curebaseupdater

import org.bouncycastle.asn1.sec.SECNamedCurves
import org.bouncycastle.crypto.digests.SHA256Digest
import org.bouncycastle.crypto.params.ECDomainParameters
import org.bouncycastle.crypto.params.ECPrivateKeyParameters
import org.bouncycastle.crypto.params.ECPublicKeyParameters
import org.bouncycastle.crypto.signers.ECDSASigner
import org.bouncycastle.crypto.signers.HMacDSAKCalculator
import java.math.BigInteger
import android.util.Log

object CureCrypto {
    private const val privHex = "E40783F681A5BB852CAB1E106B6641EFB43C1923C1EBE25CA36865CDFAB06548"

    // ✅ Usually the cube expects raw 32-byte challenge as message.
    // If you want to test "hash-before-sign" quickly, set this to true.
    private const val HASH_CHALLENGE_BEFORE_SIGN = false

    // ✅ Very common requirement in embedded verifiers: enforce low-S (s <= n/2)
    private const val LOW_S_ENFORCE = true

    private val domainParams: ECDomainParameters by lazy {
        val params = SECNamedCurves.getByName("secp256k1")
        ECDomainParameters(params.curve, params.g, params.n, params.h)
    }

    @JvmStatic
    fun buildUnlockResponse(challengeHex: String): String {
        val cleaned = challengeHex.replace(Regex("[^0-9A-Fa-f]"), "")
        require(cleaned.length == 64) { "challengeHex must be 64 hex chars (32 bytes)" }

        val challengeBytes = hexStringToByteArray(cleaned)

        val msgToSign: ByteArray = if (HASH_CHALLENGE_BEFORE_SIGN) {
            sha256(challengeBytes)
        } else {
            challengeBytes
        }

        // ✅ Make sure priv is treated as UNSIGNED and reduced mod n
        val priv = BigInteger(1, hexStringToByteArray(privHex)).mod(domainParams.n)
        val privParams = ECPrivateKeyParameters(priv, domainParams)

        val signer = ECDSASigner(HMacDSAKCalculator(SHA256Digest()))
        signer.init(true, privParams)

        val components = signer.generateSignature(msgToSign)
        var r = components[0]
        var s = components[1]

        // ✅ Enforce low-S if requested (common in firmware / strict verifiers)
        if (LOW_S_ENFORCE) {
            val n = domainParams.n
            val halfN = n.shiftRight(1)
            if (s > halfN) {
                s = n.subtract(s)
            }
        }

        val rBytes = toFixedLength(r, 32)
        val sBytes = toFixedLength(s, 32)

        val sigHex = (rBytes + sBytes).toHexString()
        Log.d(
            "CureCrypto",
            "buildUnlockResponse: challenge=$cleaned hash=$HASH_CHALLENGE_BEFORE_SIGN lowS=$LOW_S_ENFORCE sig=$sigHex (len=${sigHex.length})"
        )
        return sigHex
    }

    @JvmStatic
    fun verifyDeviceSignature(challengeHex: String, sigHex: String): Boolean {
        val cleanedChallenge = challengeHex.replace(Regex("[^0-9A-Fa-f]"), "")
        val cleanedSig = sigHex.replace(Regex("[^0-9A-Fa-f]"), "")

        if (cleanedChallenge.length != 64 || cleanedSig.length != 128) {
            Log.w("CureCrypto", "verifyDeviceSignature: Invalid input lengths")
            return false
        }

        val challenge = hexStringToByteArray(cleanedChallenge)

        val msgToVerify: ByteArray = if (HASH_CHALLENGE_BEFORE_SIGN) {
            sha256(challenge)
        } else {
            challenge
        }

        val sig = hexStringToByteArray(cleanedSig)
        val r = BigInteger(1, sig.copyOfRange(0, 32))
        val s = BigInteger(1, sig.copyOfRange(32, 64))

        val pubHex =
            "84993D74CACCB0901406255ECE4295E727A3D670918DDEA4246BBEF89FBB7BD9" +
                    "4EE1A8EDA4C0379309D9681BFE3D332E4815786B89F8F1F0F7F57DABED608116"
        val pubKeyBytes = hexStringToByteArray(pubHex)
        val x = BigInteger(1, pubKeyBytes.copyOfRange(0, 32))
        val y = BigInteger(1, pubKeyBytes.copyOfRange(32, 64))

        val ecPoint = domainParams.curve.createPoint(x, y)
        val publicKeyParams = ECPublicKeyParameters(ecPoint, domainParams)

        val signer = ECDSASigner()
        signer.init(false, publicKeyParams)

        val isValid = signer.verifySignature(msgToVerify, r, s)

        Log.d(
            "CureCrypto",
            "verifyDeviceSignature: hash=$HASH_CHALLENGE_BEFORE_SIGN challenge=$cleanedChallenge sig=${cleanedSig.take(16)}... result=$isValid"
        )
        return isValid
    }

    private fun sha256(data: ByteArray): ByteArray {
        val d = SHA256Digest()
        d.update(data, 0, data.size)
        val out = ByteArray(32)
        d.doFinal(out, 0)
        return out
    }

    private fun toFixedLength(bi: BigInteger, length: Int): ByteArray {
        val raw = bi.toByteArray()
        if (raw.size == length) return raw
        if (raw.size > length) return raw.copyOfRange(raw.size - length, raw.size)
        val out = ByteArray(length)
        System.arraycopy(raw, 0, out, length - raw.size, raw.size)
        return out
    }

    private fun hexStringToByteArray(s: String): ByteArray {
        val len = s.length
        val data = ByteArray(len / 2)
        var i = 0
        while (i < len) {
            data[i / 2] =
                ((Character.digit(s[i], 16) shl 4) + Character.digit(s[i + 1], 16)).toByte()
            i += 2
        }
        return data
    }

    private fun ByteArray.toHexString(): String {
        val sb = StringBuilder(this.size * 2)
        for (b in this) sb.append(String.format("%02x", b))
        return sb.toString()
    }
}