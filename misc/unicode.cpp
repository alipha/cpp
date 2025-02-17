#include <iostream>
#include <string>
#include <cstddef>

bool is_continuation_byte(unsigned char c) {
	return c >= 0b1000'0000 && c <= 0b1011'1111;
}

/* these 4 functions are not needed for the example, but are purely educational 
	https://en.wikipedia.org/wiki/UTF-8#Encoding */
bool is_one_byte(unsigned char c) {
	return c <= 0b0111'1111;
}

bool is_two_byte_start(unsigned char c) {
	return c >= 0b1100'0000 && c <= 0b1101'1111;
}

bool is_three_byte_start(unsigned char c) {
	return c >= 0b1110'0000 && c <= 0b1110'1111;
}

bool is_four_byte_start(unsigned char c) {
	return c >= 0b1111'0000 && c <= 0b1111'0111;
}


std::size_t count_codepoints(const std::string &s) {
	std::size_t count = 0;

	for(char c : s) {
		if(!is_continuation_byte(c)) {
			count++;
		}
	}

	return count;
}

int main() {
	std::string utf8_str = "10º很有用👍𒃵ºfoo";
	std::cout << count_codepoints(utf8_str);
}
