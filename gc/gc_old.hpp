#ifndef LIPH_GC_PTR_HPP
#define LIPH_GC_PTR_HPP

#include <cstddef>


template<typename T>
class gc_ptr;

struct gc_sentinel {};


class gc_node {
public:
    gc_node() { list_insert(head); }
    gc_node(gc_sentinel) : next(this), prev(this) {}

    virtual ~gc_node() {}

    gc_node(const gc_node &) = delete;
    gc_node(gc_node &&) = delete;
    gc_node &operator=(const gc_node &) = delete;
    gc_node &operator=(gc_node &&) = delete;

    static void free_delayed() {
        while(delayed_free_head.next != &delayed_free_head) {
            undelayed_free_count = 0;
            delete_list(delayed_free_head, false);
        }
        undelayed_free_count = 0;
    }

    static void free_unreachable() {
        // TODO: set gc_running and update ref_counts
        free_delayed();
        delete_list(head, true);

        head.next = reachable_head.next;
        head.prev = reachable_head.prev;
        head.next->prev = &head;
        head.prev->next = &head;
        reachable_head.next = &reachable_head;
        reachable_head.prev = &reachable_head;

        gc_node *node = head.next;
        while(node != &head)
            node->reachable = false;
        // TODO: gc_running = false
    }

    void mark_reachable() {
        if(reachable)
            return;

        reachable = true;
        list_remove();
        list_insert(reachable_head);
    }

protected:
    virtual void mark_children_reachable() = 0;

private:
    template<typename T>
    friend class gc_ptr;
    
    static void delete_list(gc_code &head, bool check_delayed) {
        gc_node *next = head.next;
        head.next = &head;
        head.prev = &head;

        while(next != &head) {
            gc_node *current = next;
            next = next->next;
            delete current;

            if(check_delayed)
                free_delayed();
        }
    
    }

    void free() {
        list_remove();

        if(undelayed_free_count < 1000) {
            ++undelayed_free_count;
            delete this;
        } else {
            list_insert(delayed_free_head);
        }
    }

    void list_insert(gc_code &head) {
        head.next->prev = this;
        next = head.next;
        prev = &head;
        head.next = this;
    }

    void list_remove() {
        next->prev = prev;
        prev->next = next;
    }


    std::size_t ref_count = 0;
    bool reachable = false;
    //std::size_t weak_count;
    gc_node *next;
    gc_node *prev;

    static std::size_t undelayed_free_count;
    static gc_node head;
    static gc_node delayed_free_head;
    static gc_node reachable_head;
};

/* TODO: move into cpp */
std::size_t gc_node::undelayed_free_count = 0;
gc_node gc_node::head{gc_sentinel()};
gc_node gc_node::delayed_free_head{gc_sentinel()};
gc_node gc_node::reachable_head{gc_sentinel()};
// TODO: gc_running


template<typename T>
class gc_object : public gc_node {
public:
    T &get() { return value; }

private:
    T value;
};


template<typename T>
class gc_ptr {
public:
    gc_ptr() { // TODO:
    }

    gc_ptr operator=() {  // TODO:
    }

    ~gc_ptr() {
        if(!gc_running && !--ptr->ref_count)
            ptr->free();
    }

    void mark_reachable() {
        ptr->mark_reachable();
        gc_node *old_head = reachable_head.next;

        ptr->mark_children_reachable();

        gc_node *new_head = reachable_head.next;

        while(old_head != new_head) {
            gc_node *node = new_head;

            while(node != old_head) {
                node->mark_children_reachable();
                node = node->next;
            }

            old_head = new_head;
            new_head = reachable_head.next;
        }
    }

private:
    gc_object<T> *ptr;
};


#endif
