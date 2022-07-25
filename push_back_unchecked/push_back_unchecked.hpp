#ifndef LIPH_PUSH_BACK_UNCHECKED_HPP
#define LIPH_PUSH_BACK_UNCHECKED_HPP
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
#include <vector>
#include <utility>

/* These are optimized versions to use instead of std::vector::push_back and std::vector::emplace_back
 * WHEN YOU KNOW THAT THE VECTOR WILL HAVE CAPACITY FOR THE NEW ELEMENT, to avoid std::vector from having
 * to check whether the push_back (or emplace_back) will exceed the capacity and require a larger data buffer
 * to be allocated.
 *
 * Typical usage would be to use std::vector::reserve to pre-allocate the appropriate amount of space
 * and then use push_back_unchecked or emplace_back_unchecked, as in the below example.
 *
 * Using push_back_unchecked or emplace_back_unchecked when there is not enough space reserved
 * (i.e., v.size() == v.capacity()) will result in UNDEFINED BEHAVIOR. 
 *
 * Currently, only gcc will perform this optimization. gcc 4.9.0 and higher with -O1 or higher optimization
 * will cause gcc to optimize out the capacity check: https://godbolt.org/z/nMEG7hP5q
 *
 * clang and msvc (all known versions) do not perform this optimization.
 */

/* Example usage:

std::vector<int> double_elements(const std::vector<int> &input) {
    std::vector<int> output;
    output.reserve(input.size());   // IMPORTANT: Undefined Behavior without reserving an appropriate size

    for(int element : input) {
        push_back_unchecked(output, element * 2);
    }

    return output;
}

*/

template<typename T, typename A>
constexpr void push_back_unchecked(std::vector<T, A> &v, const T &value) {
    if(v.data() + v.size() < v.data() + v.capacity()) 
    {} else { __builtin_unreachable(); }

    v.push_back(value);
}


template<typename T, typename A>
constexpr void push_back_unchecked(std::vector<T, A> &v, T &&value) {
    if(v.data() + v.size() < v.data() + v.capacity()) 
    {} else { __builtin_unreachable(); }

    v.push_back(std::move(value));
}


template<typename T, typename A, typename... Args>
constexpr void emplace_back_unchecked(std::vector<T, A> &v, Args&&... args) {
    if(v.data() + v.size() < v.data() + v.capacity()) 
    {} else { __builtin_unreachable(); }

    v.emplace_back(std::forward<Args>(args)...);
}

#endif
