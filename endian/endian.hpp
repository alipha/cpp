#ifndef LIPH_ENDIAN_HPP
#define LIPH_ENDIAN_HPP
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

However, I ask that you credit me as the author (though it's not required).

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

/* Author: Kevin Spinar (alipha) */
#include <cstdint>
#if __cplusplus >= 202002L
#include <bit>
#endif

/* gcc 8.1 and clang 9.0.0 and above optimize these to nops or bswaps with -O2.
   However, le_to_uint16 and be_to_uint16 are not optimal on gcc 8.1-9.3, but
   are optimal on 10.1 and above.
   Also, clang 9.0.0 produces non-optimal code for be_to_uint64 and le_to_uint64
   if the -march=native flag is provided. clang 10.0.0 and above are optimal.
   The excessive static_casts are to quiet -Wconversion -Wsign-conversion warnings.
 */
template<typename Char>
Char *uint64_to_be(std::uint64_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src >> 56));
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src >> 48));
    dest[2] = static_cast<Char>(static_cast<std::uint8_t>(src >> 40));
    dest[3] = static_cast<Char>(static_cast<std::uint8_t>(src >> 32));
    dest[4] = static_cast<Char>(static_cast<std::uint8_t>(src >> 24));
    dest[5] = static_cast<Char>(static_cast<std::uint8_t>(src >> 16));
    dest[6] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[7] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int64_to_be(std::int64_t src, Char *dest) {
    return uint64_to_be(static_cast<std::uint64_t>(src), dest);
}

template<typename Char>
Char *uint64_to_le(std::uint64_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[7] = static_cast<Char>(static_cast<std::uint8_t>(src >> 56));
    dest[6] = static_cast<Char>(static_cast<std::uint8_t>(src >> 48));
    dest[5] = static_cast<Char>(static_cast<std::uint8_t>(src >> 40));
    dest[4] = static_cast<Char>(static_cast<std::uint8_t>(src >> 32));
    dest[3] = static_cast<Char>(static_cast<std::uint8_t>(src >> 24));
    dest[2] = static_cast<Char>(static_cast<std::uint8_t>(src >> 16));
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int64_to_le(std::int64_t src, Char *dest) {
    return uint64_to_le(static_cast<std::uint64_t>(src), dest);
}

template<typename Char>
std::uint64_t be_to_uint64(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint64_t>(
        static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[0])) << 56
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[1])) << 48
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[2])) << 40
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[3])) << 32
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[4])) << 24
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[5])) << 16
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[6])) << 8
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[7]))
    );
}

template<typename Char>
std::int64_t be_to_int64(const Char *src) {
    return static_cast<std::int64_t>(be_to_uint64(src));
}

template<typename Char>
std::uint64_t le_to_uint64(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint64_t>(
        static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[7])) << 56
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[6])) << 48
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[5])) << 40
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[4])) << 32
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[3])) << 24
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[2])) << 16
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[1])) << 8
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(src[0]))
    );
}

template<typename Char>
std::int64_t le_to_int64(const Char *src) {
    return static_cast<std::int64_t>(le_to_uint64(src));
}


template<typename Char>
Char *uint32_to_be(std::uint32_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src >> 24));
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src >> 16));
    dest[2] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[3] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int32_to_be(std::int32_t src, Char *dest) {
    return uint32_to_be(static_cast<std::uint32_t>(src), dest);
}

template<typename Char>
Char *uint32_to_le(std::uint32_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[3] = static_cast<Char>(static_cast<std::uint8_t>(src >> 24));
    dest[2] = static_cast<Char>(static_cast<std::uint8_t>(src >> 16));
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int32_to_le(std::int32_t src, Char *dest) {
    return uint32_to_le(static_cast<std::uint32_t>(src), dest);
}

template<typename Char>
std::uint32_t be_to_uint32(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint32_t>(
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[0])) << 24
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[1])) << 16
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[2])) << 8
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[3]))
        );
}

template<typename Char>
std::int32_t be_to_int32(const Char *src) {
    return static_cast<std::int32_t>(be_to_uint32(src));
}

template<typename Char>
std::uint32_t le_to_uint32(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint32_t>(
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[3])) << 24
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[2])) << 16
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[1])) << 8
        | static_cast<std::uint32_t>(static_cast<std::uint8_t>(src[0]))
    );
}

template<typename Char>
std::int32_t le_to_int32(const Char *src) {
    return static_cast<std::int32_t>(le_to_uint32(src));
}


template<typename Char>
Char *uint16_to_be(std::uint16_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int16_to_be(std::int16_t src, Char *dest) {
    return uint16_to_be(static_cast<std::uint16_t>(src), dest);
}

template<typename Char>
Char *uint16_to_le(std::uint16_t src, Char *dest) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    dest[1] = static_cast<Char>(static_cast<std::uint8_t>(src >> 8));
    dest[0] = static_cast<Char>(static_cast<std::uint8_t>(src));
    return dest;
}

template<typename Char>
Char *int16_to_le(std::int16_t src, Char *dest) {
    return uint16_to_le(static_cast<std::uint16_t>(src), dest);
}

template<typename Char>
std::uint16_t be_to_uint16(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(static_cast<std::uint8_t>(src[0])) << 8
        | static_cast<std::uint16_t>(static_cast<std::uint8_t>(src[1]))
    );
}

template<typename Char>
std::int16_t be_to_int16(const Char *src) {
    return static_cast<std::int16_t>(be_to_uint16(src));
}

template<typename Char>
std::uint16_t le_to_uint16(const Char *src) {
    static_assert(sizeof(Char) == 1, "Char must be a byte-sized type");
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(static_cast<std::uint8_t>(src[1])) << 8
        | static_cast<std::uint16_t>(static_cast<std::uint8_t>(src[0]))
    );
}

template<typename Char>
std::int16_t le_to_int16(const Char *src) {
    return static_cast<std::int16_t>(le_to_uint16(src));
}

template<bool Big>
struct endian {
	template<typename Char> static std::uint64_t read_uint64(const Char *src) { return be_to_uint64(src); }
	template<typename Char> static std::uint32_t read_uint32(const Char *src) { return be_to_uint32(src); }
	template<typename Char> static std::uint16_t read_uint16(const Char *src) { return be_to_uint16(src); }
	template<typename Char> static std::int64_t read_int64(const Char *src) { return be_to_int64(src); }
	template<typename Char> static std::int32_t read_int32(const Char *src) { return be_to_int32(src); }
	template<typename Char> static std::int16_t read_int16(const Char *src) { return be_to_int16(src); }

	template<typename Char> static Char *write_uint64(std::uint64_t src, Char *dest) { return uint64_to_be(src, dest); }
	template<typename Char> static Char *write_uint32(std::uint32_t src, Char *dest) { return uint32_to_be(src, dest); }
	template<typename Char> static Char *write_uint16(std::uint16_t src, Char *dest) { return uint16_to_be(src, dest); }
	template<typename Char> static Char *write_int64(std::int64_t src, Char *dest) { return int64_to_be(src, dest); }
	template<typename Char> static Char *write_int32(std::int32_t src, Char *dest) { return int32_to_be(src, dest); }
	template<typename Char> static Char *write_int16(std::int16_t src, Char *dest) { return int16_to_be(src, dest); }
};

template<>
struct endian<false> {
	template<typename Char> static std::uint64_t read_uint64(const Char *src) { return le_to_uint64(src); }
	template<typename Char> static std::uint32_t read_uint32(const Char *src) { return le_to_uint32(src); }
	template<typename Char> static std::uint16_t read_uint16(const Char *src) { return le_to_uint16(src); }
	template<typename Char> static std::int64_t read_int64(const Char *src) { return le_to_int64(src); }
	template<typename Char> static std::int32_t read_int32(const Char *src) { return le_to_int32(src); }
	template<typename Char> static std::int16_t read_int16(const Char *src) { return le_to_int16(src); }

	template<typename Char> static Char *write_uint64(std::uint64_t src, Char *dest) { return uint64_to_le(src, dest); }
	template<typename Char> static Char *write_uint32(std::uint32_t src, Char *dest) { return uint32_to_le(src, dest); }
	template<typename Char> static Char *write_uint16(std::uint16_t src, Char *dest) { return uint16_to_le(src, dest); }
	template<typename Char> static Char *write_int64(std::int64_t src, Char *dest) { return int64_to_le(src, dest); }
	template<typename Char> static Char *write_int32(std::int32_t src, Char *dest) { return int32_to_le(src, dest); }
	template<typename Char> static Char *write_int16(std::int16_t src, Char *dest) { return int16_to_le(src, dest); }
};

using big = endian<true>;
using little = endian<false>;
#if __cplusplus >= 202002L
using native = endian<std::endian::native == std::endian::big>;
#endif

#endif
