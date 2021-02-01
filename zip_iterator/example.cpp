#include "zip_iterator.hpp"
#include "zip_iterator.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>



int main() {
    std::list<std::string> us{"test", "foo", "bar", "test2", "test3"};
    std::vector<int> vs{3, 6, 9};
    std::vector<double> ws{4.5, 2.5, 3.5};

    // the iterator ranges are of different lengths (5, 3, and 3), so
    // this will loop only 3 times, to prevent going past the end iterator
    for(auto &[u, v, w] : zip_range(us, vs, ws)) {
        std::cout << u << v + w << ' ';
        w /= 10;
    }
    std::cout << '\n';

    auto range = zip_range(vs, ws);

    std::sort(range.begin(), range.end(), 
        [](zip_value<int, double> left, zip_value<int, double> right) {
            return get<1>(left) < get<1>(right);
        });

    for(const auto &[v, w] : range) {
        std::cout << '(' << v << ", " << w << ") ";
    }
    std::cout << '\n';

    std::sort(range.begin(), range.end());

    // TODO: const_zip_range
    for(auto [v, w] : zip_range(vs, ws)) {
        std::cout << '(' << v << ", " << w << ") ";
    }
    std::cout << '\n';

    zip_iterator it = zip_iterator(us.begin(), vs.begin(), ws.begin() + 1);
    zip_iterator last = zip_iterator(us.end(), vs.end(), ws.end());

    // the iterator ranges are of different lengths (5, 3, and 2), so `it != last` will become false
    // as soon as one of the iterator pairs compares equal, which means this will loop only twice
    for(; it != last; ++it) {
        auto [u, v, w] = *it;
        u += 'x';
        std::cout << u << v + w << ' ';
    }
    std::cout << '\n';

    // TODO: cbegin, cend
    for(auto it = range.begin(); it < range.end(); ++it) {
        auto [v, w] = *it;
        std::cout << "from end: " << range.end() - it << "\t v + w = " << v + w << '\n'; // TODO: cend
    }
    std::cout << '\n';
//#endif
}
