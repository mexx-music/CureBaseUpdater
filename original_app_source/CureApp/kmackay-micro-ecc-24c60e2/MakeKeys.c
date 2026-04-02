/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#include "uECC.h"

#include <stdio.h>
#include <string.h>


int main() {
    int i, c;
    uint8_t private[32] = {0};
    uint8_t public[64] = {0};
    uint8_t hash[32] = {0};
    uint8_t sig[64] = {0};

    uECC_Curve curve=uECC_secp256k1();

    if (!uECC_make_key(public, private, curve)) {
        printf("uECC_make_key() failed\n");
        return 1;
    }
    
    printf("uint8_t private_key[32]={");
    for (int i=0;i<32;i++) {
        printf("0x%0.2X",private[i]);
        if (i<31)
            printf(", ");
    }

    printf("};\n");
    
    printf("uint8_t public_key[64]={");
    for (int i=0;i<64;i++) {
        printf("0x%0.2X",public[i]);
        if (i<63)
            printf(", ");
    }

    printf("};\n");

    return 0;
}
