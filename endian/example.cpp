#include "endian.hpp"

#include <cstdint>
#include <iostream>


int main() {
    signed char input[] = {-3, -5, 3, -1};
    std::cout << from_little_endian<std::int32_t>(input) << std::endl;
    std::cout << from_little_endian<std::uint32_t>(input) << std::endl;
    
    unsigned char bytes[2];
    to_big_endian(bytes, static_cast<std::int16_t>(-3));
    std::cout << static_cast<unsigned int>(bytes[0]) << ", " << static_cast<unsigned int>(bytes[1]) << std::endl;
    bytes[1]++;
    std::cout << from_big_endian<std::int16_t>(bytes) << std::endl;
    return 0;
}

