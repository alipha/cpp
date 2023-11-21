#ifndef MR_CRYPTO_SECRETBOX_H
#define MR_CRYPTO_SECRETBOX_H

#include <sodium.h>

// (Nonce) Misuse-Resistant encryption/decryption wrappers around libsodium's crypto_secretbox_* functions
//
// mr_crypto_secretbox_noauth produces a ciphertext the same size as the plaintext.
//
// If the input is at least 32 bytes, a nonce can safely be not provided or can be reused. However, if a nonce is not provided (or is reused) under the same key,
// identical input messages will produce the same ciphertext, which being able to detect if the same message was encrypted multiple times may be a vulnerability
// in certain situations. Additionally, any modification to the ciphertext will result in seemingly random data when decrypted, which may reduce the need for an
// authentication tag.
//
// If the input is less than 32 bytes, then there are fewer guarantees that similar plaintext will produce completely different ciphertext.
// If the input is n bytes and is less than 32, then there's a 1 in 2^(8*n/2) chance that two similar plaintexts will produce ciphertexts which are similar to each other.
// Similarly, if the input is n bytes and is less than 32, then there's a 1 in 2^(8*n/2) chance that a modification to the ciphertext will produce a corresponding change to the
// plaintext, instead of producing a plaintext which is completely different. And so, authentication tags would be recommended for inputs shorter than 32 bytes.
//
// You should consider a block cipher or Format Preserving Encrypting, if you wish to encrypt input less than 32 bytes without requiring a nonce or auth tag.
//
// Encrypts `len` bytes at `msg` and stores the resulting `len` bytes at `ciphertext`. 
// ciphertext may equal msg. Otherwise, `ciphertext` and `msg` may not overlap.
// nonce can be NULL. If not NULL, it must point to a byte array of size crypto_stream_xchacha20_NONCEBYTES.
// key must point to a byte array of size `crypto_stream_xchacha20_KEYBYTES`
void mr_crypto_secretbox_noauth(unsigned char *ciphertext, const unsigned char *msg, size_t len, const unsigned char *nonce, const unsigned char *key);

// Decrypts `len` bytes at `ciphertext` and stores the resulting `len` bytes at `plaintext`. 
// ciphertext may equal plaintext. Otherwise, `ciphertext` and `plaintext` may not overlap.
// nonce can be NULL. If not NULL, it must point to a byte array of size crypto_stream_xchacha20_NONCEBYTES.
// key must point to a byte array of size `crypto_stream_xchacha20_KEYBYTES`
void mr_crypto_secretbox_noauth_open(unsigned char *plaintext, const unsigned char *ciphertext, size_t len, const unsigned char *nonce, const unsigned char *key);


#endif
