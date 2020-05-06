#ifndef LIPH_GC_HPP
#define LIPH_GC_HPP

#include "gc_detail.hpp"
#include <utility>
#include <type_traits>


// TODO: const correctness?
// TODO: use allocators?
// TODO: separate node from T
// TODO: add before_destroy
// TODO: exception safe


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

    virtual bool detail_perform(detail::node *node); // { node.mark_reachable(); }
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
    ptr() : p(nullptr) {}

    template<typename... Args>
    ptr(std::in_place_t, Args&&... args) 
        : p(new detail::object<T, Transverse>(std::forward<Args>(args)...)) {}

    ptr(const ptr &other) : p(other.p) { ++p->ref_count; }
    ptr(ptr &&other) : p(other.p) { other.p = nullptr; }

    ptr &operator=(const ptr &other) {
        reset();
        p = other.p;
        ++p->ref_count;
        return *this;
    }

    ptr &operator=(ptr &&other) {
        reset();
        p = other.p;
        other.p = nullptr;
        return *this;
    }

    ~ptr() { reset(); }

    void reset() {
        if(p && !detail::is_running && !--p->ref_count)
            p->free();
        p = nullptr;
    }

    template<typename... Args>
    static ptr make(Args&&... args) {
        return 
    }
    
private:
    detail::object<T, Transverse> *p;
};


template<typename T, typename Transverse = transverse_children<T>>
class anchor_ptr : public detail::anchor_node {
public:
    anchor_ptr() = default;

    anchor_ptr(const ptr<T, Transverse> &p) : p(p) {}
    anchor_ptr(ptr<T, Transverse> &&p) : p(std::move(p)) {}

    anchor_ptr(const anchor_ptr &other) : detail::anchor_node(), p(other.p) {}
    anchor_ptr(anchor_ptr &&other) : detail::anchor_node(), p(std::move(other.p)) {}

    anchor_ptr &operator=(const anchor_ptr &other) {
        p = other.p;
        return *this;
    }
    
    anchor_ptr &operator=(anchor_ptr &&other) {
        p = std::move(other.p);
        return *this;
    }

protected:
    node *get_node() override { return p.p; } 

private:
    ptr<T> p;
};


template<typename T, typename Transverse = transverse_children<T>, typename A
}  // namespace gc

#endif

