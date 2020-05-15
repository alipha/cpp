#include "gc.hpp"
#include <iostream>
#include <vector>


struct foo {
    ~foo() { std::cout << "~foo()" << std::endl; }
    void transverse(gc::action &act) { act(next); } 
    gc::ptr<foo> next;
};


int main() {
    std::vector<int, gc::allocator<int>> v;

    v.push_back(3);
    std::cout << "push_back: " << gc::get_memory_used() << std::endl;
    
    gc::anchor_ptr<foo> f = gc::make_ptr<foo>();
    f->next = f;
    f.reset();

    std::cout << "foo: " << gc::get_memory_used() << std::endl;

    gc::set_memory_limit(gc::get_memory_used());
    v.push_back(5);
    
    std::cout << "push_back: " << gc::get_memory_used() << std::endl;
    v.pop_back();
    v.shrink_to_fit();
    std::cout << "shrink: " << gc::get_memory_used() << std::endl;

    std::vector<int, gc::allocator<int>> v2 = v;
    std::cout << gc::get_memory_used() << std::endl;
}
