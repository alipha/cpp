#ifndef LIPH_GC_DETAIL_HPP
#define LIPH_GC_DETAIL_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
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


template<typename T>
struct ptr;

template<typename T>
struct anchor_ptr;

struct action;


namespace detail {


using std::begin;
using std::end;

template<typename T, typename = std::void_t<>>
constexpr bool is_container_v = false;

template<typename T>
constexpr bool is_container_v<T, std::void_t<decltype(begin(std::declval<T&>()), end(std::declval<T&>()))>> = true;


template<typename T, typename = std::void_t<>>
constexpr bool has_transverse_v = false;

template<typename T>
constexpr bool has_transverse_v<T, std::void_t<decltype(std::declval<T>().transverse(std::declval<action&>()))>> = true;


template<typename T, typename = std::void_t<>>
constexpr bool has_before_destroy_v = false;

template<typename T>
constexpr bool has_before_destroy_v<T, std::void_t<decltype(std::declval<T>().before_destroy())>> = true;



template<typename T>
struct do_action;

template<template<typename> typename Func>
struct apply_to_all;


struct node;


}  // namespace detail



template<typename T>
struct transverse {
    template<typename U = T>
    std::enable_if_t<detail::has_transverse_v<U>> operator()(T &obj, action &act) { 
        obj.transverse(act); 
    }
};


template<typename T>
struct before_destroy {
    template<typename U = T>
    std::enable_if_t<detail::has_before_destroy_v<U>> operator()(T &obj) { obj.before_destroy(); }

    template<typename U = T>
    std::enable_if_t<!detail::has_before_destroy_v<U>> operator()(T &) {}
};

/*
template<typename T>
struct transverse<T&> : transverse<T> {};

template<typename T>
struct transverse<T&&> : transverse<T> {};
*/



template<typename T>
struct transverse<ptr<T>> {
    void operator()(ptr<T> &p, action &act) { act(p); }
};


template<typename T>
struct transverse<anchor_ptr<T>> {
    void operator()(anchor_ptr<T> &p, action &act) { act(p); }
};



struct action {
    template<typename T>
    void operator()(T &obj) {
        detail::apply_to_all<detail::do_action>()(obj, *this);
    }

    virtual bool detail_perform(detail::node *node) = 0;
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


template<typename T>
struct do_action {
    template<typename U>
    void operator()(ptr<U> &p, action &act) {
        if(p.p)
            act.detail_perform(p.p);
    }
};


template<typename T, template<typename> typename Func, typename... Args>
constexpr bool can_apply(long) { return false; }

template<typename T, template<typename> typename Func, typename... Args>
constexpr bool can_apply(int, std::void_t<decltype(Func<T>()(std::declval<T&>(), std::declval<Args&>()...))>* = 0) { return true; }


template<typename T, template<typename> typename Func, typename... Args>
constexpr bool can_apply_to_all(long) { return false; }

template<typename T, template<typename> typename Func, typename... Args>
constexpr bool can_apply_to_all(int, std::void_t<decltype(apply_to_all<Func>()(std::declval<T&>(), std::declval<Args&>()...))>* = 0) { return true; }



template<template<typename> typename Func>
struct apply_to_all {
    template<typename T, typename... Args>
    std::enable_if_t<!detail::is_container_v<T> && can_apply<T, Func, Args...>(0)> 
    operator()(T &obj, Args&&... args) {
        Func<T>()(obj, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    std::enable_if_t<detail::is_container_v<T>> operator()(T &container, Args&&... args) {
        for(auto &obj : container)
            apply_to_all<Func>()(obj, std::forward<Args>(args)...);
    }

    template<typename... Ts, typename... Args>
    void operator()(std::variant<Ts...> &v, Args&... args) {
        static_assert((false || ... || can_apply_to_all<Ts, Func, Args...>(0)), 
                "expected at least one T in std::variant to be transversable");

        std::visit([&](auto &obj) {
            if constexpr(can_apply_to_all<decltype(obj), Func, Args...>(0)) {
                debug_out("can apply");
                apply_to_all<Func>()(obj, args...);
            } else {
                debug_out("CAN'T apply");
            }
        }, v);
    }

    template<typename T, typename U, typename... Args>
    void operator()(std::pair<T, U> &p, Args&&... args) {
        constexpr bool can_apply_T = can_apply_to_all<T, Func, Args...>(0);
        constexpr bool can_apply_U = can_apply_to_all<U, Func, Args...>(0);
        static_assert(can_apply_T || can_apply_U, "expected one of the types in the std::pair to be transversable");

        if constexpr(can_apply_T)
            apply_to_all<Func>()(p.left, args...);
        if constexpr(can_apply_U)
            apply_to_all<Func>()(p.right, args...);
    }
};



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
struct object : node {
    template<typename... Args>
    object(Args&&... args) : node(), value(std::forward<Args>(args)...) {}

    void transverse(action &act) override { apply_to_all<gc::transverse>()(value, act); }
    void before_destroy() override { apply_to_all<gc::before_destroy>()(value); }

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

