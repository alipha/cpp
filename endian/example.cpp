#include "endian.hpp"

#include <cstdint>
#include <iostream>


int main() {
    signed char input[] = {-3, -5, 3, -1};
    std::cout << le_to_uint32(input) << std::endl;
    
    unsigned char bytes[2];
    uint16_to_be(-3, bytes);
    std::cout << static_cast<unsigned int>(bytes[0]) << ", " << static_cast<unsigned int>(bytes[1]) << std::endl;
    bytes[1]++;
    std::cout << be_to_uint16(bytes) << std::endl;
    return 0;
}

