#include <sodium.h>
#include <cstdint>
#include <array>
#include <iostream>
#include <map>
#include <iomanip>
#include <utility>
#include <cstring>

   
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
         encrypt(ciphertext, current, current_size, seed.data());
         sodium_increment(current, (current_size + 1) / 8); 
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
         max >>= 1;
      }
      return size < 1 ? 1 : size;
   }
   
	static std::pair<size_t, unsigned char> len_and_mask(size_t bit_len) {
		if(bit_len % 8)
			return {bit_len / 8 + 1, static_cast<unsigned char>((1U << (bit_len % 8)) - 1)};
		else
			return {bit_len / 8, 255};
	}
/*
	struct split_info {
		size_t split_index;
		unsigned char part1_mask;
		unsigned char part2_first_mask;	// at part2[]
		size_t part2_len;
		unsigned char part2_last_mask;	// at part2[split_index + part2_len]
	};

	static std::pair<size_t, unsigned char> len_and_mask(size_t bit_len) {
			return {bit_len / 8 + 1, static_cast<unsigned char>((1U << (bit_len % 8)) - 1)};
		else
			return {bit_len / 8, 255};
	}
*/
	static void stream_xor(unsigned char *dest, unsigned char *src, size_t msg_off, size_t msg_bit_len, unsigned char *hashtext, size_t hash_bit_len, const unsigned char *key, size_t step) {
		unsigned char subkey[crypto_stream_xchacha20_KEYBYTES];
		
		auto [hash_bytelen, hash_mask] = len_and_mask(hash_bit_len);
		hashtext[hash_bytelen-1] &= hash_mask;
		crypto_generichash(subkey, sizeof subkey, hashtext, hash_bytelen, key, crypto_stream_xchacha20_KEYBYTES);

		subkey[0] &= 0xfc;
		subkey[0] |= step;
		
		auto [msg_bytelen, msg_mask] = len_and_mask(msg_bit_len);
		crypto_stream_xchacha20_xor(dest + msg_off, src + msg_off, msg_bytelen, zero_nonce, subkey);
		dest[msg_off + msg_bytelen - 1] &= msg_mask;
	}

	static void encrypt(unsigned char *ciphertext, unsigned char *msg, size_t bit_len, const unsigned char *key) {
		if(bit_len == 0) {
			return;
		}
		if(bit_len == 1) {
			unsigned char plaintext = msg[0] & 1;
			crypto_stream_xchacha20_xor(ciphertext, &plaintext, 1, zero_nonce, key);
			ciphertext[0] &= 1;
			return;
		}

		size_t part1_len;
		size_t part2_len = 256;	// 32 bytes

		if(bit_len < part2_len * 2) {
			part2_len = bit_len / 2;
		} else {
			// part2_len will be at most 33 bytes (32 bytes + 7 bits) (not important)
			part2_len += bit_len % 8;	// make part1_len be divisible by 8
		}

		part1_len = bit_len - part2_len;

		if(part1_len % 8 == 0) {
			size_t part1_off = part1_len / 8;
			stream_xor(ciphertext, msg,        0,         part1_len, msg + part1_off,        part2_len, key, 1);
			stream_xor(ciphertext, msg,        part1_off, part2_len, ciphertext,             part1_len, key, 2);
			stream_xor(ciphertext, ciphertext, 0,         part1_len, ciphertext + part1_off, part2_len, key, 3);
		} else {
			unsigned char part1[32];	// there will be up to 31 full bytes, then a partial byte on the right side
			unsigned char part2[33];	// there will be up to 31 full bytes, then a partial byte on both sides
			size_t split_index = part1_len / 8;
			size_t part1_extra_bitcount = part1_len % 8;
			size_t part2_extra_left_bitcount = 8 - part1_extra_bitcount;
			unsigned char part2_left_mask = static_cast<unsigned char>(~((1U << part1_extra_bitcount) - 1));
			size_t part2_len_remaining = part2_len - part2_extra_left_bitcount;					// 26 = [ 13 ][ 13 ] = [8][5 3][8][2]
			size_t part2_extra_right_bitcount = part2_len_remaining % 8;
			size_t part2_bytelen = (part2_len_remaining + 7) / 8 + 1;
			std::memcpy(part1, msg, split_index + 1);
			std::memcpy(part2, msg + split_index, part2_bytelen);			// 24 = [ 12 ][ 12 ] = [8][4 4][8]
			part2[0] &= part2_left_mask;

			size_t part2_len_for_xor = part2_len_remaining + 8;
			stream_xor(part1, part1, 0, part1_len,         part2, part2_len_for_xor, key, 1);
			stream_xor(part2, part2, 0, part2_len_for_xor, part1, part1_len,         key, 2);
			part2[0] &= part2_left_mask;
			stream_xor(part1, part1, 0, part1_len,         part2, part2_len_for_xor, key, 3);

			part2[0] |= part1[split_index];
			std::memcpy(ciphertext, part1, split_index);
			std::memcpy(ciphertext + split_index, part2, part2_bytelen);
		}
	}

	inline static unsigned char zero_nonce[crypto_stream_xchacha20_NONCEBYTES];

   seed_type seed;
   std::uint64_t max;
   unsigned char current[8];
   int current_size;
};


int main() {
   std::map<int, int> m;
   random_sequence seq(1270);
   for(int i = 0; i <= 1280; ++i) {
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
}

