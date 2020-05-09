#include "gc.hpp"

#include <iostream>
#include <cstddef>
#include <chrono>


struct node {
    node(std::size_t id, bool show_destroy) : id(id), show_destroy(show_destroy) {}

    void transverse(gc::action &act) { act(next); }

    void before_destroy() { if(show_destroy) std::cout << 'b' << id << ' '; }

    ~node() { if(show_destroy) std::cout << '~' << id << ' '; }

    std::size_t id;
    bool show_destroy;
    gc::ptr<node> next;
};


gc::ptr<node> build_list(std::size_t count, bool show_destroy, bool make_loop) {
    gc::anchor_ptr<node> root = gc::make_ptr<node>(0, show_destroy);
    gc::ptr<node> current = root;

    for(std::size_t i = 1; i < count; ++i) {
        current->next = gc::make_ptr<node>(i, show_destroy);
        current = current->next;
    }

    if(make_loop)
        current->next = root;

    return root;
}


void deletion_test(std::size_t count, bool show_destroy) {
    gc::anchor_ptr<node> root1 = build_list(count, show_destroy, false);
    std::cout << "built list: " << count << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now(); 
    root1 = nullptr;
    auto end = std::chrono::high_resolution_clock::now(); 

    std::cout << "set nullptr" << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Time: " << time_taken / 1e6 << "ms" << std::endl;

    gc::anchor_ptr<node> root2 = build_list(count, show_destroy, true);
    std::cout << "built list: " << count << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;
    root2 = nullptr;
    std::cout << "set nullptr" << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;

    start = std::chrono::high_resolution_clock::now(); 
    gc::collect();
    end = std::chrono::high_resolution_clock::now(); 
    time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Time: " << time_taken / 1e6 << "ms" << std::endl;
}


int main() {
    deletion_test(45, true);
    deletion_test(55, true);
    deletion_test(30000000, false);
    return 0;
}
