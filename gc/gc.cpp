#include "gc.hpp"


namespace gc {


namespace detail {


node head{sentinel()};
node delayed_free_head{sentinel()};
node reachable_head{sentinel()};
anchor_node anchor_head{sentinel()};
bool is_running = false;


struct mark_reachable_action : action {
    bool detail_perform(detail::node *node) override { return node->mark_reachable(); }
};


struct free_action : action {
    free_action(std::size_t d) : depth(d) {}

    bool detail_perform(detail::node *node) override {
        if(node->ref_count > 1) {
            --node->ref_count;
            return false;
        }

        node->list_remove();
        node->before_destroy();

        if(depth > 50) {
            node->list_insert(delayed_free_head);
        } else {
            free_action child_action(depth + 1);
            node->transverse(child_action);
            delete node;
        }

        return true;
    }

    std::size_t depth;
};


void transverse_and_mark_reachable(node *ptr) {
    mark_reachable_action act;

    if(!ptr || !act.detail_perform(ptr))
        return;
    
    node *old_head = reachable_head.next;
    ptr->transverse(act);

    node *new_head = reachable_head.next;

    while(old_head != new_head) {
        node *node = new_head;

        while(node != old_head) {
            node->transverse(act);
            node = node->next;
        }

        old_head = new_head;
        new_head = reachable_head.next;
    }
}


void free_delayed() {
    free_action act(0);

    while(delayed_free_head.next != &delayed_free_head) {
        node *next = delayed_free_head.next;
        delayed_free_head.next = &delayed_free_head;
        delayed_free_head.prev = &delayed_free_head;

        while(next != &delayed_free_head) {
            node *current = next;
            next = next->next;
            current->transverse(act);
            delete current;
        }
    }
}


void free_unreachable() {
    is_running = true;
    delete_list(head);
    is_running = false;

    if(reachable_head.next != &reachable_head) {
        head.next = reachable_head.next;
        head.prev = reachable_head.prev;
        head.next->prev = &head;
        head.prev->next = &head;
        reachable_head.next = &reachable_head;
        reachable_head.prev = &reachable_head;
    } else {
        head.next = &head;
        head.prev = &head;
    }

    node *n = head.next;
    while(n != &head) {
        n->reachable = false;
        n = n->next;
    }
}


void delete_list(node &head) {
    node *next = head.next;

    while(next != &head) {
        next->before_destroy();
        next = next->next;
    }

    next = head.next;
    //head.next = &head;
    //head.prev = &head;

    while(next != &head) {
        node *current = next;
        next = next->next;
        delete current;
    }
}


bool node::mark_reachable() {
    if(reachable) {
        ++ref_count;
        return false;
    }

    ref_count = 1;
    reachable = true;
    list_remove();
    list_insert(reachable_head);
    return true;
}


void node::free() {
    is_running = true;
    free_action(0).detail_perform(this);
    free_delayed();
    is_running = false;
}


}  // detail



void collect() {
    detail::mark_reachable_action act;
    detail::anchor_node *node = detail::anchor_head.next;

    while(node != &detail::anchor_head) {
        detail::transverse_and_mark_reachable(node->detail_get_node());
        node = node->next;
    }

    detail::free_unreachable();
}


}  // namespace gc

