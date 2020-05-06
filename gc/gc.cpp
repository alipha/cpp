#include "gc.hpp"


namespace gc {


namespace detail {


std::size_t undelayed_free_count = 0;
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
        if(node->ref_count > 1)
            return false;

        node->list_remove();

        if(depth > 50) {
            node->list_insert(delayed_free_head);
        } else {
            free_action child_action(depth + 1);
            node->transverse_children(child_action);
            delete node;
        }

        return true;
    }

    std::size_t depth;
};


void mark_reachable(node *ptr) {
    mark_reachable_action act;

    if(!act.detail_perform(*ptr))
        return; //ptr->mark_reachable();
    
    node *old_head = reachable_head.next;

    ptr->transverse_children(act);
    //ptr->mark_children_reachable();

    node *new_head = reachable_head.next;

    while(old_head != new_head) {
        node *node = new_head;

        while(node != old_head) {
            node->transverse_children(act);
            //node->mark_children_reachable();
            node = node->next;
        }

        old_head = new_head;
        new_head = reachable_head.next;
    }
}


void free_delayed() {   // TODO: revisit
    while(delayed_free_head.next != &delayed_free_head) {
        undelayed_free_count = 0;
        delete_list(delayed_free_head, false);
    }
    undelayed_free_count = 0;
}


void free_unreachable() {
    // TODO: set running and update ref_counts
    free_delayed();
    delete_list(head, true);

    head.next = reachable_head.next;
    head.prev = reachable_head.prev;
    head.next->prev = &head;
    head.prev->next = &head;
    reachable_head.next = &reachable_head;
    reachable_head.prev = &reachable_head;

    node *n = head.next;
    while(n != &head) {
        n->reachable = false;
        n = n->next;
    }
    // TODO: running = false
}


void delete_list(node &head, bool check_delayed) {   // TODO: not exception safe :-/
    node *next = head.next;
    head.next = &head;
    head.prev = &head;

    while(next != &head) {
        node *current = next;
        next = next->next;
        delete current;

        if(check_delayed)
            free_delayed();
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
    gc_running = true;
    free_action action(0);
    gc_running = false;
}


}  // detail



void collect() {
    mark_reachable_action act;
    anchor_node *node = anchor_head.next;

    while(node != &anchor_head) {
        mark_reachable(node->get_node());
        node = node->next;
    }

    free_unreachable();
}


}  // namespace gc

