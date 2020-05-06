#ifndef LIPH_GC_HPP
#define LIPH_GC_HPP

#include "gc_detail.hpp"
#include <type_traits>


// TODO: const correctness?
// TODO: use allocators?


namespace gc {


void collect();


struct action {
    template<typename T>
    void operator()(ptr<T> &p) { detail_perform(p.p); }

    template<typename T>
    std::enable_if_t<detail::is_container_v<T>> operator()(T &container) {
        for(auto &obj : container)
            operator()(obj);
    }

    virtual bool detail_perform(detail::node &node); // { node.mark_reachable(); }
};


template<typename T>
struct transverse_children {
    template<typename U = T>
    std::enable_if_t<!detail::is_container_v<U>> operator()(T &obj, action &act) { 
        obj.transverse_children(act); 
    }
    
    template<typename U = T>
    std::enable_if_t<detail::is_container_v<U>> operator()(T &container, action &act) {
       act(container); 
    }
};


template<typename T, typename Transverse = transverse_children<T>>
class ptr {
public:
    gc_ptr() { // TODO:
    }

    gc_ptr operator=() {  // TODO:
    }

    ~gc_ptr() {
        if(!gc_running && !--ptr->ref_count)
            ptr->free();
    }
    
private:
    detail::object<T, Transverse> *p;
};


template<typename T>
class anchor_ptr : public detail::anchor_node {
public:
    
private:
    ptr<T> p;
};


}  // namespace gc

#endif

