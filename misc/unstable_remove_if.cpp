#include <iostream>
#include <vector>


template<typename BiDirIt, typename Func>
BiDirIt unstable_remove_if(BiDirIt first, BiDirIt last, Func &&func) {
    if(first == last) return first;
    --last;

    while(first != last) {
        if(func(*first)) {
            *first = std::move(*last);
            --last;
        } else {
            ++first;
        }
    }

    if(!func(*first))
        ++first;

    return first;
}

template<typename BiDirIt, typename Value>
BiDirIt unstable_remove(BiDirIt first, BiDirIt last, const Value &value) {
    return unstable_remove_if(first, last, [&value](auto &&x) { return x == value; });
}


int main() {
    std::vector<int> v{5, 12, 14, 17, 21, 23, 30, 32, 36, 37, 40, 41};
    auto last = unstable_remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });

    for(auto it = v.begin(); it != last; ++it) {
        std::cout << *it << ' ';
    }

    std::cout << '\n';
    
    for(int x : v) {
        std::cout << x << ' ';
    }
}

