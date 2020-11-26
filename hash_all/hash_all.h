#ifndef LIPH_HASH_ALL_H
#define LIPH_HASH_ALL_H

#include <cstddef>
#include <functional>


std::size_t hash_all() { return 0; }

template<typename T>
std::size_t hash_all(const T &t) { return std::hash<T>()(t); }

template<typename First, typename... Rest>
std::size_t hash_all(const First &first, const Rest &... rest) {
    return std::hash<First>()(first) + 31 * hash_all(rest...);
}

#endif
