#include "hash_all.h"

#include <iostream>
#include <string>
#include <set>
#include <tuple>
#include <unordered_set>


struct person {
    int id;
    std::string first_name;
    std::string last_name;
};


bool operator==(const person &lhs, const person &rhs) {
    return std::tie(lhs.id, lhs.first_name, lhs.last_name) ==
           std::tie(rhs.id, rhs.first_name, rhs.last_name);
}

bool operator<(const person &lhs, const person &rhs) {
    return std::tie(lhs.id, lhs.first_name, lhs.last_name) <
           std::tie(rhs.id, rhs.first_name, rhs.last_name);
}

namespace std {
    template<>
    struct hash<person> {
        size_t operator()(const person &p) const { 
            return hash_all(p.id, p.first_name, p.last_name);
        }
    };
}


int main() {
    std::cout << hash_all() << '\n';
    std::cout << hash_all(3) << '\n';
    std::cout << hash_all(3, 5, 2) << '\n';
    std::cout << hash_all(5.2) << '\n';
    std::cout << hash_all(std::string("foo")) << '\n';
    std::cout << hash_all(std::string("foo"), 3) << '\n';
    std::cout << hash_all(3, std::string("foo")) << "\n\n";
    
    person kevin = {4, "kevin", "smith"};
    person john = {8, "john", "doe"};
    person steve = {1, "steve", "wonder"};
    std::set<person> people = {kevin, john};
    std::unordered_set<person> people2 = {kevin, steve};
    
    std::cout << people.find(kevin)->id << '\n';
    std::cout << people2.find(steve)->id << '\n';
}

