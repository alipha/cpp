#ifndef LIPH_RANDOM_SEQUENCE_H
#define LIPH_RANDOM_SEQUENCE_H

#include "mr_crypto_secretbox.h"
#include <cstdint>
#include <array>


class random_sequence {
public:
	// should be 32 bytes
	using seed_type = std::array<unsigned char, crypto_stream_xchacha20_KEYBYTES>;

	// generate a non-repeating sequence between 0 and max, inclusive
	random_sequence(std::uint64_t max) : max(max), current{}, current_size(get_current_size(max)) {
		crypto_secretbox_keygen(seed.data());
	}

	random_sequence(std::uint64_t max, seed_type seed) : seed(seed), max(max), current{}, current_size(get_current_size(max)) {}

	std::uint64_t next() {
		std::uint64_t result;
		unsigned char ciphertext[8]{};

		do {
			mr_crypto_secretbox_noauth(ciphertext, current, current_size, nullptr, seed.data());
			sodium_increment(current, current_size);
			result = ciphertext[0]
				| (static_cast<std::uint64_t>(ciphertext[1]) << 8)
				| (static_cast<std::uint64_t>(ciphertext[2]) << 16)
				| (static_cast<std::uint64_t>(ciphertext[3]) << 24)
				| (static_cast<std::uint64_t>(ciphertext[4]) << 32)
				| (static_cast<std::uint64_t>(ciphertext[5]) << 40)
				| (static_cast<std::uint64_t>(ciphertext[6]) << 48)
				| (static_cast<std::uint64_t>(ciphertext[7]) << 56);
		} while(result > max);

		return result;
	}

private:
	static int get_current_size(std::uint64_t max) {
		int size = 0;

		while(max) {
			++size;
			max >>= 8;
		}
		return size < 2 ? 2 : size;
	}

	seed_type seed;
	std::uint64_t max;
	unsigned char current[8];
	int current_size;
};

#endif
