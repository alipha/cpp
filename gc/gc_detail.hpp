#ifndef LIPH_GC_DETAIL_HPP
#define LIPH_GC_DETAIL_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


namespace gc {


struct action;

template<typename T>
struct transverse_children;

template<typename T, typename Transverse = transverse_children<T>>
class ptr;



namespace detail {


struct node;
struct anchor_node;

struct sentinel {};


extern std::size_t undelayed_free_count;
extern node head;
extern node delayed_free_head;
extern node reachable_head;
extern anchor_node anchor_head;
extern bool is_running;


void mark_reachable(node *ptr);
void free_delayed();
void free_unreachable();
void delete_list();


template<typename T, typename = std::void_t<>>
constexpr bool is_container_v = false;

template<typename T>
constexpr bool is_container_v<T, std::void_t<decltype(std::begin(std::declval<T>()), std::end(std::declval<T>()))>> = true;


template<typename Node>
struct list_node {
    list_node() {}
    list_node(Node *n, Node *p) : next(n), prev(p) {}

    void list_insert(Node &head) {
        head.next->prev = this;
        next = head.next;
        prev = &head;
        head.next = this;
    }

    void list_remove() {
        next->prev = prev;
        prev->next = next;
    }

    Node *next;
    Node *prev;
};


struct node : list_node<node> {
    node() { list_insert(head); }
    node(sentinel) : list_node(this, this) {}

    virtual ~node() {}

    node(const node &) = delete;
    node(node &&) = delete;
    node &operator=(const node &) = delete;
    node &operator=(node &&) = delete;

    virtual void transverse_children(action &act) = 0;

    bool mark_reachable();

    void free();

    std::size_t ref_count = 0;
    bool reachable = false;
};



template<typename T, typename Transverse>
struct object : node {
    using node::node;

    void transverse_children(action &act) override { Transverse()(value, act); }

    T value;
};


struct anchor_node : list_node<anchor_node> {
    anchor_node(node *n) : n(n) { list_insert(anchor_head); }
    anchor_node(sentinel) : n(nullptr), list_node(this, this) {}

    node *n;
};


}  // namespace detail

}  // namespace gc

#endif

