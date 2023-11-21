#include "mr_crypto_secretbox.h"
#include <sodium.h>

#if crypto_generichash_BYTES_MIN > crypto_stream_xchacha20_KEYBYTES || crypto_generichash_BYTES_MAX < crypto_stream_xchacha20_KEYBYTES
#error "libsodium's implementation of hash and key sizes are incompatible"
#endif

#if crypto_stream_xchacha20_KEYBYTES > crypto_generichash_KEYBYTES_MAX
#error "libsodium's xchacha20 KEYBYTES is larger than allowed generichash KEYBYTES"
#endif


unsigned char zero_nonce[crypto_stream_xchacha20_NONCEBYTES];


static void mr_stream_xor(unsigned char *dest, const unsigned char *src, size_t msg_off, size_t msg_len, const unsigned char *hashtext, size_t hashtext_len, const unsigned char *nonce, const unsigned char *key, size_t step) {
    unsigned char subkey[crypto_stream_xchacha20_KEYBYTES];

    crypto_generichash(subkey, sizeof subkey, hashtext, hashtext_len, key, crypto_stream_xchacha20_KEYBYTES);
    subkey[0] &= 0xfc;
    subkey[0] |= step;

    crypto_stream_xchacha20_xor(dest + msg_off, src + msg_off, msg_len, nonce, subkey);
}


/*
 * This divides the plaintext into two parts, part1 is 32 bytes and part2 is the remainder of the plaintext.
 * (if the plaintext is smaller than 64 bytes, then the two parts are instead divided evenly.)
 *
 * Three encryptions are performed:
 * 1. part1 is encrypted with a key generated from hash(key, part2)
 * 2. part2 is encrypted with a key generated from hash(key, part1)       [note: this is the encrypted part1 from step 1]
 * 3. part1 is encrypted again with a key generated from hash(key, part2) [note: this is the encrypted part2 from step 2]
 *
 * Note then that the key used in step 2 has been influenced by every bit in the plaintext because it is based upon a
 * hash of part1, which part1 has been encrypted using a key based upon part2, so a change to any bit in part1 or part2 would
 * affect the key generated for step 2.
 *
 * Thusfar, part1 has been encrypted with a key which has only been influenced by part2, and so we need step 3 to
 * make the encryption of part1 to be influenced by every bit of part1 also.
 */
void mr_crypto_secretbox_noauth(unsigned char *ciphertext, const unsigned char *msg, size_t len, const unsigned char *nonce, const unsigned char *key) {
    size_t part1_len = 32;
    size_t part2_len;
    const unsigned char *n = nonce ? nonce : zero_nonce;

    if(len < part1_len * 2) {
        part1_len = len / 2;
    }

    part2_len = len - part1_len;

    if(len == 0) {
        return;
    }
    if(part1_len == 0) {
        crypto_stream_xchacha20_xor(ciphertext, msg, len, n, key);
        return;
    }

    mr_stream_xor(ciphertext, msg, 0, part1_len, msg + part1_len, part2_len, n, key, 1);
    mr_stream_xor(ciphertext, msg, part1_len, part2_len, ciphertext, part1_len, n, key, 2);
    mr_stream_xor(ciphertext, ciphertext, 0, part1_len, ciphertext + part1_len, part2_len, n, key, 3);
}


void mr_crypto_secretbox_noauth_open(unsigned char *plaintext, const unsigned char *ciphertext, size_t len, const unsigned char *nonce, const unsigned char *key) {
    return mr_crypto_secretbox_noauth(plaintext, ciphertext, len, nonce, key);
}

