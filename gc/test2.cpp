#include "gc.hpp"

#include <iostream>
#include <cstddef>
#include <chrono>


struct pnode {
    pnode(std::size_t id, bool show_destroy) : id(id), show_destroy(show_destroy) {}

    ~pnode() { if(show_destroy) std::cout << '~' << id << ' '; }

    std::size_t id;
    bool show_destroy;
    pnode *next;
};



struct node {
    node(std::size_t id, bool show_destroy) : id(id), show_destroy(show_destroy) {}

    void transverse(gc::action &act) { act(next); }

    void before_destroy() { if(show_destroy) std::cout << 'b' << id << ' '; }

    ~node() { if(show_destroy) std::cout << '~' << id << ' '; }

    std::size_t id;
    bool show_destroy;
    gc::ptr<node> next;
};


pnode *build_plist(std::size_t count) {
    pnode *root = new pnode(0, false);
    pnode *current = root;

    for(std::size_t i = 1; i < count; ++i) {
        current->next = new pnode(i, false);
        current = current->next;
    }

    current->next = nullptr;
    return root;
}


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
    auto start = std::chrono::high_resolution_clock::now(); 
    gc::anchor_ptr<node> root1 = build_list(count, show_destroy, false);    
    auto end = std::chrono::high_resolution_clock::now(); 

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Build Time: " << time_taken / 1e6 << "ms" << std::endl;

    std::cout << "built list: " << count << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;
   

    start = std::chrono::high_resolution_clock::now(); 
    gc::ptr<node> prev = std::move(root1);
    gc::ptr<node> current = std::move(prev->next);
    prev->next = nullptr;

    while(current) {
        gc::ptr<node> next = std::move(current->next);
        current->next = std::move(prev);
        prev = std::move(current);
        current = std::move(next);
    }
    root1 = std::move(prev);
    end = std::chrono::high_resolution_clock::now(); 
    time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Reverse List: " << time_taken / 1e6 << "ms" << std::endl;


    start = std::chrono::high_resolution_clock::now(); 
    root1 = nullptr;
    end = std::chrono::high_resolution_clock::now(); 

    std::cout << "set nullptr" << std::endl;
    std::cout << "nodes: " << gc::object_count() << ", anchors: " << gc::anchor_count() << std::endl;

    time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Delayed Free Time: " << time_taken / 1e6 << "ms" << std::endl;



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
    std::cout << "Collect Time: " << time_taken / 1e6 << "ms" << std::endl;
}


int main() {
    deletion_test(5, true);
    deletion_test(30000000, false);


    auto start = std::chrono::high_resolution_clock::now(); 
    pnode *root1 = build_plist(30000000);    
    auto end = std::chrono::high_resolution_clock::now(); 

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Classic Build Time: " << time_taken / 1e6 << "ms" << std::endl;


    start = std::chrono::high_resolution_clock::now(); 
    pnode *prev = root1;
    pnode *current = prev->next;
    prev->next = nullptr;

    while(current) {
        pnode *next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    root1 = prev;
    end = std::chrono::high_resolution_clock::now(); 
    time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Classic Reverse List: " << time_taken / 1e6 << "ms" << std::endl;


    start = std::chrono::high_resolution_clock::now(); 
    pnode *next = root1;

    while(next) {
        pnode *current = next;
        next = next->next;
        delete current;
    }
    
    end = std::chrono::high_resolution_clock::now(); 

    time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();   
    std::cout << "Classic Delete Time: " << time_taken / 1e6 << "ms" << std::endl;
    return 0;
}
