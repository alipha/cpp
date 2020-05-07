#ifndef LIPH_GC_DETAIL_HPP
#define LIPH_GC_DETAIL_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <variant>

#ifdef DEBUG
#include <stdexcept>
#include <string>
#endif

#ifdef DEBUG_OUT
#include <iostream>

#define debug_out(x) do { std::cerr << "DEBUG: " << x << std::endl; } while(0)
#define debug_error(x) do { std::cerr << "ERROR: " << x << std::endl; } while(0)
#else
#define debug_out(x) do {} while(0)
#define debug_error(x) do {} while(0)
#endif

constexpr bool debug = false
#ifdef DEBUG
    || true
#endif
    ;


namespace gc {

struct action;


namespace detail {


using std::begin;
using std::end;

template<typename T, typename = std::void_t<>>
constexpr bool is_container_v = false;

template<typename T>
constexpr bool is_container_v<T, std::void_t<decltype(begin(std::declval<T>()), end(std::declval<T>()))>> = true;


template<typename T, typename = std::void_t<>>
constexpr bool has_before_destroy_v = false;

template<typename T>
constexpr bool has_before_destroy_v<T, std::void_t<decltype(std::declval<T>().before_destroy())>> = true;


}  // namespace detail


template<typename T>
struct transverse {
    template<typename U = T>
    std::enable_if_t<!detail::is_container_v<U>> operator()(T &obj, action &act) { 
        obj.transverse(act); 
    }
    
    template<typename U = T>
    std::enable_if_t<detail::is_container_v<U>> operator()(T &container, action &act) {
        for(auto &obj : container)
            transverse<decltype(obj)>()(obj, act); 
    }
};


template<typename T, typename = std::void_t<>>
constexpr bool can_transverse_v = false;

template<typename T>
constexpr bool can_transverse_v<T, std::void_t<decltype(transverse<T>()(std::declval<T>(), std::declval<action>()))>> = true;


template<typename... T>
struct transverse<std::variant<T...>> {
    void operator()(std::variant<T...> &v, action &act) {
        std::visit([&act](auto &&obj) {
            if constexpr(can_transverse_v<decltype(obj)>) {
                transverse<decltype(obj)>()(obj, act);
            }
        }, v);
    }
};

template<typename... T>
struct transverse<std::variant<T...>&> : transverse<std::variant<T...>> {};


template<typename T>
struct before_destroy {
    template<typename U = T>
    std::enable_if_t<detail::has_before_destroy_v<U>> operator()(T &obj) { obj.before_destroy(); }

    template<typename U = T>
    std::enable_if_t<detail::is_container_v<U>> operator()(T &container) {
        for(auto &obj : container)
            before_destroy<decltype(obj)>()(obj); 
    }

    template<typename U = T>
    std::enable_if_t<!detail::has_before_destroy_v<U> && !detail::is_container_v<U>> operator()(T &) {}
};



namespace detail {


struct node;
struct anchor_node;

struct sentinel {};


extern node active_head;
extern node delayed_free_head;
extern node reachable_head;
extern anchor_node anchor_head;
extern bool is_running;


void debug_not_head(node *n, node *allowed_head);
void mark_reachable(node *ptr);
void free_delayed();
void free_unreachable();
void delete_list(node &head);



template<typename Node>
struct list_node {
    list_node() noexcept {}
    list_node(Node *n, Node *p) noexcept : next(n), prev(p) {}

    list_node(const list_node &) = delete;
    list_node(list_node &&) = delete;
    list_node &operator=(const list_node &) = delete;
    list_node &operator=(list_node &&) = delete;

    void list_insert(Node &head) {
        head.next->prev = static_cast<Node*>(this);
        next = head.next;
        prev = &head;
        head.next = static_cast<Node*>(this);
    }

    void list_remove() {
        next->prev = prev;
        prev->next = next;
    }

    Node *next;
    Node *prev;
};


struct node : list_node<node> {
    node() noexcept { list_insert(active_head); }
    node(sentinel) noexcept : list_node(this, this) {}
    virtual ~node() {}

    virtual void transverse(action &) {}
    virtual void before_destroy() {}

    void list_remove() {
        if(debug)
            debug_not_head(this, nullptr);
        list_node<node>::list_remove();
    }

    bool mark_reachable();
    void free();

    std::size_t ref_count = 1;
    bool reachable = false;
};



template<typename T>
struct object : transverse<T>, before_destroy<T>, node {
    template<typename... Args>
    object(Args&&... args) : node(), value(std::forward<Args>(args)...) {}

    void transverse(action &act) override { gc::transverse<T>::operator()(value, act); }
    void before_destroy() override { gc::before_destroy<T>::operator()(value); }

    T value;
};


struct anchor_node : list_node<anchor_node> {
    anchor_node() noexcept { list_insert(anchor_head); }
    anchor_node(sentinel) noexcept : list_node(this, this) {}

    ~anchor_node() { list_remove(); }

    virtual node *detail_get_node() noexcept(!debug) { 
#ifdef DEBUG
        throw std::logic_error("anchor_node::detail_get_node called");
#else
        return nullptr; 
#endif
    }
};



}  // namespace detail

}  // namespace gc

#endif

