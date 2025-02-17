//#include "mr_crypto_secretbox.h"
#include "random_sequence.h"
#include <iostream>
#include <map>
#include <iomanip>

int main() {
	std::map<int, int> m;
	random_sequence seq(900000);
	for(int i = 0; i <= 900001; ++i) {
		if(i % 40 == 0)
			std::cout << '\n';
		int result = seq.next();
		std::cout << '\t' << result;
		if(m.find(result) != m.end()) {
			std::cout << "dup! " << i << " = " << std::hex << result << '\n';
			return 1;
		}
		m[result] = i;
	}
	std::cout << '\n';
	/*
	std::map<int, int> m;
	unsigned char key[crypto_secretbox_KEYBYTES];
	unsigned char ciphertext[2];
	unsigned char plaintext[2];
	crypto_secretbox_keygen(key);

	for(int i = 0; i < 65536; ++i) {
		plaintext[0] = i & 255;
		plaintext[1] = i >> 8;
		mr_crypto_secretbox_noauth(ciphertext, plaintext, 2, nullptr, key);
		int result = (ciphertext[0] | (ciphertext[1] << 8));
		if(m.find(result) != m.end()) {
			std::cout << "dup! " << i << " = " << std::hex << result << '\n';
			return 1;
		}
		m[result] = i;
		std::cout << i << " = " << std::hex << result << '\n';
	}*/
}
