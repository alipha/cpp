#include <https://raw.githubusercontent.com/alipha/cpp/master/endian/endian.hpp>
#include <iostream>
#include <cstdint>
#include <cstring>


float big_endian_to_float(const std::uint8_t *be_bytes) {
    std::uint32_t host_bytes = be_to_uint32(be_bytes);
    float float_value;
    std::memcpy(&float_value, &host_bytes, sizeof(float_value));
    return float_value;
}

void float_to_big_endian(float float_value, std::uint8_t *be_bytes) {
    std::uint32_t host_bytes;
    std::memcpy(&host_bytes, &float_value, sizeof(float_value));
    uint32_to_be(host_bytes, be_bytes);
}


int main() {
    std::uint8_t data[4];
    
    float_to_big_endian(3.25, data);
    std::cout << +data[0] << ' ' << +data[1] << ' ' << +data[2] << ' ' << +data[3] << std::endl;

    std::cout << big_endian_to_float(data) << std::endl;
}
