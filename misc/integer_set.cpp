#include <cstddef>
#include <set>


struct integer_range {
    integer_range(std::size_t value) : first(value), last(value) {}

    mutable std::size_t first;
    mutable std::size_t last;
};

bool operator<(integer_range left, integer_range right) {
    return left.first < right.first;
}


struct integer_set {
    integer_set(integer_range init) : ranges{init} {}

    bool insert(std::size_t value) {
        auto up_it = ranges.upper_bound(value);
        if(up_it != ranges.end()) {
            if(up_it->first == value + 1) {
                if(up_it != ranges.begin() && std::prev(up_it)->last == value) {
                    auto low = std::prev(up_it)->first;
                    ranges.erase(std::prev(up_it));
                    up_it->first = low;
                    return true;
                } else {
                    up_it->first = value;
                    return true;
                }
            }
        
}
        // up_it == end || up_it->first > value + 1
        if(up_it != ranges.begin()) {
            auto low_it = std::prev(up_it);
            if(low_it->last == value) {
                std::prev(up_it)->last = value + 1;
                return true;
            } else if(low_it->last > value) {
                return false;
            }
        }
        ranges.insert(up_it, value);
        return true;
    }

    bool remove(std::size_t value) {
        
    }

    std::set<integer_range> ranges;
};

