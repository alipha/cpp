// MIT License
#ifndef LIPH_ENDIAN_HPP
#define LIPH_ENDIAN_HPP

#include <cstddef>
#include <limits>
#include <type_traits>

/*
 *  gcc 8.1 optimizes all of these to simple MOVs or BSWAPs.
 *  clang 9.0 optimizes the to_X_endian functions but not the from_X_endian functions.
 *
 *  Example usage:
 *
 *   // read 32-bit unsigned int from file, increment value, and write it back out
 *   char bytes[4];
 *   std::fstream file("filename", std::ios::binary);
 *   file.read(bytes, 4);
 *   std::uint32_t value = from_little_endian<std::uint32_t>(bytes);
 *   to_little_endian(bytes, value + 1);
 *   file.write(bytes, 4);
 */
template<typename Integer, typename Char>
Char* to_little_endian(Char *dest, Integer src) {
    static_assert(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>);
    static_assert(sizeof(Char) == 1 && !std::is_same_v<Char, bool>);

    using UInt = std::make_unsigned_t<Integer>;

    constexpr int char_bits = std::numeric_limits<unsigned char>::digits;
    constexpr unsigned char mask = std::numeric_limits<unsigned char>::max();

    UInt s = src;

    for(std::size_t i = 0; i < sizeof(UInt); ++i) {
        dest[i] = (s >> (i * char_bits)) & mask;
    }

    return dest;
}

template<typename Integer, typename Char>
Char* to_big_endian(Char *dest, Integer src) {
    static_assert(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>);
    static_assert(sizeof(Char) == 1 && !std::is_same_v<Char, bool>);

    using UInt = std::make_unsigned_t<Integer>;

    constexpr int char_bits = std::numeric_limits<unsigned char>::digits;
    constexpr unsigned char mask = std::numeric_limits<unsigned char>::max();

    UInt s = src;

    for(std::size_t i = 0; i < sizeof(UInt); ++i) {
        dest[sizeof(UInt) - i - 1] = (s >> (i * char_bits)) & mask;
    }

    return dest;
}

template<typename Integer, typename Char>
Integer from_little_endian(const Char *src) {
    static_assert(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>);
    static_assert(sizeof(Char) == 1 && !std::is_same_v<Char, bool>);

    using UInt = std::make_unsigned_t<Integer>;
    using UChar = std::make_unsigned_t<Char>;

    constexpr int char_bits = std::numeric_limits<unsigned char>::digits;
    UInt dest = 0;

    for(std::size_t i = 0; i < sizeof(UInt); ++i) {
        dest |= static_cast<UInt>(static_cast<UChar>(src[i])) << (i * char_bits);
    }

    return dest;
}

template<typename Integer, typename Char>
Integer from_big_endian(const Char *src) {
    static_assert(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>);
    static_assert(sizeof(Char) == 1 && !std::is_same_v<Char, bool>);

    using UInt = std::make_unsigned_t<Integer>;
    using UChar = std::make_unsigned_t<Char>;

    constexpr int char_bits = std::numeric_limits<unsigned char>::digits;
    UInt dest = 0;

    for(std::size_t i = 0; i < sizeof(UInt); ++i) {
        dest |= static_cast<UInt>(static_cast<UChar>(src[sizeof(UInt) - i - 1])) << (i * char_bits);
    }

    return dest;
}

#endif
