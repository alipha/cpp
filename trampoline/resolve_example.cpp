#include "trampoline.hpp"
#include <iostream>
#include <memory>
#include <deque>
#include <queue>

using liph::trampoline;


struct node {
    node(int val) : value(val) {}

    int value;
    std::unique_ptr<node> left;
    std::unique_ptr<node> right;
};


trampoline<std::unique_ptr<node>> build(std::queue<int> &values) {
    if(values.empty()) {
        return nullptr;
    } else {
        auto n = std::make_unique<node>(values.front());
        values.pop();

        auto step = [&values]() { return build(values); };

        return trampoline([n = std::move(n)](std::unique_ptr<node> right, std::unique_ptr<node> left) mutable {
            n->left = std::move(left);
            n->right = std::move(right);
            return std::move(n);
        }, step, step);
    }
}


trampoline<bool> display(const node *n) {
    if(!n) {
        return false;
    } else {
        auto left = [n]() { return display(n->left.get()); };
        auto right = [n]() { return display(n->right.get()); };

        return trampoline([n](auto, auto) {
            std::cout << n->value << std::endl;
            return false;
        }, left, right);
    }
}


trampoline<const node*> search(const node *n, int value) {
    std::cout << "Searching: " << (n ? n->value : -1) << std::endl;
    if(!n) {
        return nullptr;
    } else if(n->value == value) {
        return trampoline<const node*>::resolve(n);
    } else {
        auto result1 = [n, value]() { return search(n->left.get(), value); };
        auto result2 = [n, value]() { return search(n->right.get(), value); };
        return trampoline<const node*>([](auto, auto) { return nullptr; }, result1, result2);
    }
}


// just a simple example of trampolines calling trampolines of other types and
// then resolving a different type than the trampoline type you're inside of.
trampoline<int> sum_int(int n);

trampoline<long> sum_long(long n) {
    if(n <= 1) {
        return trampoline<long>::resolve((int)n);   // even with the cast, resolve(long) is called
    } else {
        auto result1 = [n]() { return sum_int(n - 1); };
        return trampoline<long>([n](int &&n1) { return n + n1; }, result1);
    }
}

trampoline<int> sum_int(int n) {
    if(n <= 1) {
        return trampoline<int>::resolve<long>(n);
    } else {
        auto result1 = [n]() { return sum_long(n - 1); };
        return trampoline<int>([n](long &&n1) { return n + n1; }, result1);
    }
}



int main() {
    std::deque<int> list = {3, 6, 9, 12, 15, 18, 21, 24, 27, 30};
    std::queue<int> values(list);

    std::unique_ptr<node> tree = build(values).run_breadth();

    display(tree.get()).run();
    std::cout << std::endl;
    display(tree.get()).run_breadth();
    std::cout << std::endl;

    const node *n = search(tree.get(), 12).run();
    std::cout << "Search result: " << (n ? n->value : -1) << std::endl;

    n = search(tree.get(), 18).run_breadth();
    std::cout << "Search result: " << (n ? n->value : -1) << std::endl;

    n = search(tree.get(), 999).run();
    std::cout << "Search result: " << (n ? n->value : -1) << std::endl;


    std::cout << std::endl;
    std::cout << sum_long(5).run() << std::endl;
    std::cout << sum_long(4).run() << std::endl;

    try {
        sum_int(5).run();
    } catch(liph::trampoline_resolver_type_mismatch &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }
    try {
        sum_int(4).run();
    } catch(liph::trampoline_resolver_type_mismatch &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }
}

