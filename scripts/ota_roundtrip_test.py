#!/usr/bin/env python3
import zlib
from Crypto.Cipher import AES
from Crypto.Util import Counter

# OtaEncoder.encode behavior (from OtaEncoder.kt)
# For each byte b:
# unsigned = b & 0xFF
# if unsigned in [0x00, 0x0A, 0x0D, ord('=')]: output '=' then ((unsigned + 42 + 64) & 0xFF)
# else output (unsigned + 42) & 0xFF
# output is interpreted as ISO-8859-1 string (latin-1)

def ota_encode(raw_bytes: bytes) -> str:
    out = bytearray()
    for b in raw_bytes:
        unsigned = b
        if unsigned in (0x00, 0x0A, 0x0D, ord('=')):
            out.append(ord('='))
            v = (unsigned + 42 + 64) & 0xFF
            out.append(v)
        else:
            v = (unsigned + 42) & 0xFF
            out.append(v)
    return out.decode('latin-1')

def ota_decode(text: str) -> bytes:
    data = text.encode('latin-1')
    out = bytearray()
    i = 0
    while i < len(data):
        v = data[i]
        if v == ord('='):
            # next byte contains (unsigned + 42 + 64) & 0xFF
            i += 1
            if i >= len(data):
                raise ValueError('invalid escape at end')
            vv = data[i]
            unsigned = (vv - 42 - 64) & 0xFF
            out.append(unsigned)
        else:
            unsigned = (v - 42) & 0xFF
            out.append(unsigned)
        i += 1
    return bytes(out)

# AES-CTR helpers using pycryptodome
def aes_ctr_encrypt(key: bytes, iv: bytes, data: bytes) -> bytes:
    # Build counter from IV (big endian)
    # Crypto.Util.Counter.new expects little-endian by default? We'll use int.from_bytes and new with initial_value
    iv_int = int.from_bytes(iv, byteorder='big')
    ctr = Counter.new(128, initial_value=iv_int)
    cipher = AES.new(key, AES.MODE_CTR, counter=ctr)
    return cipher.encrypt(data)

def aes_ctr_decrypt(key: bytes, iv: bytes, data: bytes) -> bytes:
    # symmetric
    iv_int = int.from_bytes(iv, byteorder='big')
    ctr = Counter.new(128, initial_value=iv_int)
    cipher = AES.new(key, AES.MODE_CTR, counter=ctr)
    return cipher.decrypt(data)

def roundtrip_test():
    # build raw of about 10 KB
    raw = (b'HELLOHELLO' * 1024)[:10240]
    print('raw len', len(raw))

    # zlib compress: use zlib.compress -> zlib wrapper (windowbits default yields zlib)
    z = zlib.compress(raw)
    print('zlib len', len(z))

    # AES key/iv from OtaCrypto.kt (converted from hex bytes)
    key = bytes([0x2f,0x71,0x66,0xb9,0x4c,0x8f,0x00,0xff,0x0a,0x88,0x9c,0x5b,0xb7,0x74,0x3c,0xb4,0x52,0x8f,0xd6,0x04,0x2c,0x75,0xec,0xf0,0x15,0x14,0xc6,0x6e,0x8f,0x49,0xd2,0x81])
    iv = bytes([0xfe,0x71,0x28,0x16,0xad,0x43,0x89,0x65,0x91,0xfe,0x03,0xd8,0x9c,0xba,0xe2,0xdd])

    enc = aes_ctr_encrypt(key, iv, z)
    print('enc len', len(enc))

    txt = ota_encode(enc)
    print('encoded len', len(txt))

    # decode
    decoded = ota_decode(txt)
    print('decoded len', len(decoded))

    # decrypt
    dec = aes_ctr_decrypt(key, iv, decoded)
    print('decrypted len', len(dec))

    # inflate
    inflated = zlib.decompress(dec)
    print('inflated len', len(inflated))

    print('equal', inflated == raw)
    return inflated == raw

if __name__ == '__main__':
    ok = roundtrip_test()
    if not ok:
        raise SystemExit(2)
    else:
        print('ROUNDTRIP OK')
