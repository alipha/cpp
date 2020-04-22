#include "trampoline.hpp"

#include <iostream>
#include <vector>

using liph::trampoline;
using liph::trampoline_list;


struct tree_node {
    int value;
    std::vector<tree_node> children;
};


trampoline<bool> display(const tree_node &n) {

    trampoline_list<bool> trampolines;
    for(const tree_node &child : n.children)
        trampolines.add([&child]() { return display(child); });

    return trampoline<bool>([&n](auto &) {
        std::cout << n.value << std::endl;
        return false;
    }, trampolines);
}


trampoline<int> sum(const tree_node &n) {

    trampoline_list<int> trampolines;
    for(const tree_node &child : n.children)
        trampolines.add([&child]() { return sum(child); });

    return trampoline<int>([&n](const std::vector<int> &sums) {
        int total = n.value;
        for(const int &s : sums)
            total += s;
        return total;
    }, trampolines);
}



int main() {
    std::vector<tree_node> grandchildren1{{522, {}}, {524, {}}};
    std::vector<tree_node> grandchildren2{{562, {}}};
    std::vector<tree_node> children{{52, grandchildren1}, {54, {}}, {56, grandchildren2}};
    tree_node tree{5, children};

    display(tree).run();
    std::cout << std::endl;
    display(tree).run_breadth();
    std::cout << std::endl;
    std::cout << sum(tree).run() << std::endl;
}
