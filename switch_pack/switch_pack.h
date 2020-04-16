#ifndef SWITCH_PACK_H
#define SWITCH_PACK_H

#include <cstddef>
#include <cstdint>
#include <string>


namespace liph {


constexpr std::uint64_t pack_overflow() noexcept { return ~static_cast<std::uint64_t>(0); }
std::uint64_t pack_overflow_non_constexpr() noexcept { return pack_overflow(); }


constexpr std::uint64_t pack(const char *str) noexcept {
    std::uint64_t result = 0;
    std::size_t i = 0;
    
    for(; str[i] && i < 8; ++i)
        result |= (static_cast<std::uint64_t>(str[i]) & 0xff) << (i * 8);
    
    return str[i] ? pack_overflow_non_constexpr() : result;
}


std::uint64_t pack(const std::string &str) noexcept { return pack(str.c_str()); }


}  // namespace liph

#endif
