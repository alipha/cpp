#include "mr_crypto_secretbox.h"
#include <stddef.h>
#include <stdio.h>

void print_hex(unsigned char *str, size_t len) {
	for(size_t i = 0; i < len; ++i) {
		printf("%02x ", str[i]);
	}
	putchar('\n');
}

int main(int argc, char **argv) {
	char plaintext1[] = "hello, world!";
	unsigned char ciphertext1[sizeof plaintext1] = {0};

	char plaintext2[] = "This is a message longer than 64 characters to test the if branch of forcing one part to be at most 32 characters.";
	unsigned char ciphertext2[sizeof plaintext2] = {0};

	unsigned char plaintext3 = 'b';
	unsigned char ciphertext3;

	unsigned char key[crypto_stream_xchacha20_KEYBYTES];
	crypto_secretbox_keygen(key);

	mr_crypto_secretbox_noauth(ciphertext1, (unsigned char*)plaintext1, sizeof plaintext1 - 1, NULL, key);
	if(argc > 1) ciphertext1[argc == 2 ? 2 : 10] = 3;
	print_hex(ciphertext1, sizeof ciphertext1);

	mr_crypto_secretbox_noauth_open(ciphertext1, ciphertext1, sizeof plaintext1 - 1, NULL, key);
	puts((char*)ciphertext1);


	mr_crypto_secretbox_noauth(ciphertext2, (unsigned char*)plaintext2, sizeof plaintext2 - 1, NULL, key);
	if(argc > 1) ciphertext2[argc == 2 ? 1 : 110] = 3; 
	print_hex(ciphertext2, sizeof ciphertext2);

	mr_crypto_secretbox_noauth_open(ciphertext2, ciphertext2, sizeof plaintext2 - 1, NULL, key);
	puts((char*)ciphertext2);


	mr_crypto_secretbox_noauth(&ciphertext3, &plaintext3, 1, NULL, key);
	print_hex(&ciphertext3, 1);

	mr_crypto_secretbox_noauth_open(&ciphertext3, &ciphertext3, 1, NULL, key);
	printf("%c\n", ciphertext3);
}
