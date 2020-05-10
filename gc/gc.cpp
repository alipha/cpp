#include "gc.hpp"

#ifdef DEBUG
using namespace std::string_literals;
#endif


namespace gc {


bool run_on_bad_alloc = true;


namespace detail {


node active_head{sentinel()};
node delayed_free_head{sentinel()};
node reachable_head{sentinel()};
anchor_node anchor_head{sentinel()};
bool is_running = false;


void debug_not_head(node *n, node *allowed_head) {
    if(!debug)
        return;
    if(!n)
        throw std::logic_error("debug_not_head: node is null");
    if(n == allowed_head)
        return;

    if(n == &active_head)
        throw std::logic_error("node is active_head");
    if(n == &delayed_free_head)
        throw std::logic_error("node is delayed_free_head");
    if(n == &reachable_head)
        throw std::logic_error("node is reachable_head");
}


struct mark_reachable_action : action {
    // returning true means: did i do something? false means it was already marked
    bool detail_perform(detail::node *node) override { 
        if(debug && !node)
            throw std::logic_error("mark_reachable_action: null");
        return node->mark_reachable(); 
    }
};

// for debug only
struct dec_ref_action : action {
    bool detail_perform(detail::node *node) override { 
        if(!node)
            throw std::logic_error("dec_ref_action: null");
        if(node->ref_count == 0)
            debug_error("dec_ref_action: ref_count is 0");
        else
            --node->ref_count;
        return true;
    } 
};


struct free_action : action {
    // returning true means: did i do something? false means this object is not ready to be freed (ref_count > 0)
    bool detail_perform(detail::node *node) override {
        if(debug && !node)
            throw std::logic_error("free_action: null");

        if(node->ref_count > 1) {
            --node->ref_count;
            return false;
        }

		if(debug)
			node->ref_count = 0;
        node->list_remove();
        //node->before_destroy();
        node->list_insert(delayed_free_head);
        return true;
    }
};


void transverse_list(node &head, node *old_head, action &act) {
    node *new_head = head.next;

    while(old_head != new_head) {
        //debug_out("start transverse_list loop");
        node *node = new_head;

        while(node != old_head) {
            debug_not_head(node, nullptr);
            node->transverse(act);
            node = node->next;
        }

        old_head = new_head;
        new_head = head.next;
    }
}


void transverse_and_mark_reachable(node *ptr) {
    mark_reachable_action act;

    if(!ptr || !act.detail_perform(ptr))
        return;
    
    node *old_head = reachable_head.next;
    ptr->transverse(act);

	transverse_list(reachable_head, old_head, act);
}


void free_delayed() {
    free_action act;

	node *old_head = &delayed_free_head;
    transverse_list(delayed_free_head, old_head, act);

	delete_list(delayed_free_head);
    delayed_free_head.next = &delayed_free_head;
    delayed_free_head.prev = &delayed_free_head;
}


void free_unreachable() {
    is_running = true;
    delete_list(active_head);
    is_running = false;

    if(reachable_head.next != &reachable_head) {
        debug_out("still reachable nodes");
        active_head.next = reachable_head.next;
        active_head.prev = reachable_head.prev;
        active_head.next->prev = &active_head;
        active_head.prev->next = &active_head;
        debug_not_head(active_head.next, nullptr);
        debug_not_head(active_head.prev, nullptr);

        reachable_head.next = &reachable_head;
        reachable_head.prev = &reachable_head;
    } else {
        debug_out("no reachable nodes");
        active_head.next = &active_head;
        active_head.prev = &active_head;
    }

    debug_out("free_unreachable: setting reachable = false");
    node *n = active_head.next;
    while(n != &active_head) {
        debug_not_head(n, nullptr);
        if(debug && !n->reachable)
            debug_error("free_unreachable: n is not reachable");
        n->reachable = false;
        n = n->next;
    }
}


void delete_list(node &head) {
    dec_ref_action dec_action;
    node *next = head.next;

    debug_out("call before_destroy");
    while(next != &head) {
        debug_not_head(next, nullptr);
        next->before_destroy();
        if(debug && &head == &active_head)
            next->transverse(dec_action);
        next = next->next;
    }

    next = head.next;

    debug_out("deleting list");
    while(next != &head) {
        debug_not_head(next, nullptr);
        if(debug && next->ref_count != 0)
            debug_error("delete_list ref_count = " + std::to_string(next->ref_count));

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

    if(debug && ref_count != 0)
        throw std::logic_error("free: refcount is not 0!");
    free_action().detail_perform(this);
    free_delayed();

    is_running = false;
}


}  // detail



void collect() {
    detail::mark_reachable_action act;
    detail::anchor_node *node = detail::anchor_head.next;

    debug_out("collect: marking reachable nodes: " + std::to_string(object_count())
            + ", anchors: " + std::to_string(anchor_count()));

    while(node != &detail::anchor_head) {
        detail::transverse_and_mark_reachable(node->detail_get_node());
        node = node->next;
    }

    debug_out("collect: freeing unreachables");
    detail::free_unreachable();
    
    debug_out("collect: still reachable nodes: " + std::to_string(object_count())
            + ", anchors: " + std::to_string(anchor_count()));
}


std::size_t object_count() {
    if(debug) {
        if(detail::delayed_free_head.next != &detail::delayed_free_head)
            debug_error("delayed_free is not empty");
        if(detail::delayed_free_head.prev != &detail::delayed_free_head)
            debug_error("delayed_free.prev != head");
        if(detail::reachable_head.next != &detail::reachable_head)
            debug_error("reachable is not empty");
        if(detail::reachable_head.prev != &detail::reachable_head)
            debug_error("reachable.prev != head");
    }

    std::size_t count = 0;
    detail::node *next = detail::active_head.next;

    while(next != &detail::active_head) {
        detail::debug_not_head(next, nullptr);
        ++count;
        next = next->next;
    }

   return count; 
}


std::size_t anchor_count() {
    std::size_t count = 0;
    detail::anchor_node *next = detail::anchor_head.next;

    while(next != &detail::anchor_head) {
        ++count;
        next = next->next;
    }

   return count; 
}


}  // namespace gc

