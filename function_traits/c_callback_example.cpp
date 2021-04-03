#include "c_callback.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>


struct comparer {
    int asc(const void *left, const void *right) { 
        ++counter;
        int arg1 = *(const int*)left;
        int arg2 = *(const int*)right;

        if (arg1 < arg2) return -1;
        if (arg1 > arg2) return 1;
        return 0;
    }
    
    template<typename T>
    int desc(T left, T right) { 
        return -asc(left, right);
    }
    
    int counter = 0;
};


int main() {
    comparer c;
    
    // specifying the type of fp_asc is unnecessary. i could have used `auto fp_asc = ...`, but i wanted
    // to make it explicit that, yes, fp_asc is a regular function pointer.
    // same for fp_desc, fp_desc2, lamb_old, and lamb
    int (*fp_asc)(const void *, const void *) = liph::make_c_callback(&c, &comparer::asc);
    
    // if we were done with fp_asc, then we wouldn't need the `<1>` and could just "overwrite"
    // the existing callback. passing distinct N's to liph::make_c_callback enables multiple callbacks
    // to be active at the same time.
    int (*fp_desc)(const void *, const void *) = liph::make_c_callback<1>(&c, &comparer::desc<const void*>);
    
    // can use liph::make_c_callback_as to specify which function overload should be used (or what template arguments should resolve to, in this case)
    int (*fp_desc2)(const void *, const void *) = liph::make_c_callback_as<int(const void*, const void*), 2>(&c, &comparer::desc);
    

    int (*lamb_old)(const void *, const void *) = liph::make_c_callback([fp_asc](const void *left, const void *right) { return -fp_asc(left, right); });
    
    // can use liph::make_c_callback_as to specify what the `auto` parameters should be
    int (*lamb)(const void *, const void *) = liph::make_c_callback_as<int(const void*, const void*)>([fp_asc](auto left, auto right) { return -fp_asc(left, right); });
    
    
    int ints[] = { -2, 99, 0, -743, 2, -1000, 4 };
    std::size_t size = sizeof ints / sizeof *ints;
 
    std::qsort(ints, size, sizeof(int), fp_asc);
    
    std::cout << "calls: " << c.counter << '\n';
    c.counter = 0;
    
    for(int i : ints)
        std::cout << i << ' ';
    std::cout << '\n';
    
    std::qsort(ints, size, sizeof(int), lamb);

    std::cout << "calls: " << c.counter << '\n';
    for(int i : ints)
        std::cout << i << ' ';
    std::cout << '\n';

}
