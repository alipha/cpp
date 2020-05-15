#include "gc.hpp"
#include <limits>

#ifdef DEBUG
using namespace std::string_literals;
#endif


namespace gc {


bool run_on_bad_alloc = true;


namespace detail {


node active_head;
node temp_head;
anchor_node anchor_head{sentinel()};

bool is_running = false;
bool is_retrying = false;
std::size_t nested_create_count = 0;
std::size_t memory_used = 0;
std::size_t memory_limit = std::numeric_limits<std::size_t>::max();


void debug_not_head(node *n, node *allowed_head) {
    if(!debug)
        return;
    if(!n)
        throw std::logic_error("debug_not_head: node is null");
    if(n == allowed_head)
        return;

    if(n == &active_head)
        throw std::logic_error("node is active_head");
    if(n == &temp_head)
        throw std::logic_error("node is temp_head");
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
        node->list_insert(temp_head);
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


void transverse_and_mark_reachable(node *ptr, action &act) {
    if(!ptr || !act.detail_perform(ptr))
        return;
    
    node *old_head = temp_head.next;
    ptr->transverse(act);

	transverse_list(temp_head, old_head, act);
}


void transverse_and_mark_reachable(node *ptr) {
    mark_reachable_action act;
    transverse_and_mark_reachable(ptr, act);
}


void free_delayed() {
    free_action act;

	node *old_head = &temp_head;
    transverse_list(temp_head, old_head, act);

	delete_list(temp_head);
    temp_head.next = &temp_head;
    temp_head.prev = &temp_head;
}


void free_unreachable() {
    is_running = true;
    delete_list(active_head);
    is_running = false;

    if(temp_head.next != &temp_head) {
        debug_out("still reachable nodes");
        active_head.next = temp_head.next;
        active_head.prev = temp_head.prev;
        active_head.next->prev = &active_head;
        active_head.prev->next = &active_head;
        debug_not_head(active_head.next, nullptr);
        debug_not_head(active_head.prev, nullptr);

        temp_head.next = &temp_head;
        temp_head.prev = &temp_head;
    } else {
        debug_out("no reachable nodes");
        active_head.next = &active_head;
        active_head.prev = &active_head;
    }

    //debug_out("free_unreachable: setting reachable = false");
    reset_reachable_flag(active_head);
}


void reset_reachable_flag(node &head) {
    node *n = head.next;
    while(n != &head) {
        debug_not_head(n, nullptr);
        if(debug && nested_create_count == 0 && !n->reachable)
            debug_error("free_unreachable: n is not reachable");
        n->reachable = false;
        n = n->next;
    }
}


void delete_list(node &head) {
    dec_ref_action dec_action;
    node *next = head.next;

    //debug_out("call before_destroy");
    while(next != &head) {
        debug_not_head(next, nullptr);
        next->before_destroy();
        if(debug && &head == &active_head)
            next->transverse(dec_action);
        next = next->next;
    }

    next = head.next;

    //debug_out("deleting list");
    while(next != &head) {
        debug_not_head(next, nullptr);
        if(debug && next->ref_count != 0)
            debug_error("delete_list ref_count = " + std::to_string(next->ref_count));

        node *current = next;
        next = next->next;
        memory_used -= current->get_memory_used();
        delete current;
    }
}


void move_temp_to_active() {
    if(temp_head.next != &temp_head) {
        //debug_out("moving temp list to front of head");
        temp_head.next->prev = &active_head;
        temp_head.prev->next = active_head.next;
        active_head.next->prev = temp_head.prev;
        active_head.next = temp_head.next;
        debug_not_head(active_head.next, nullptr);
        debug_not_head(active_head.prev, nullptr);

        temp_head.next = &temp_head;
        temp_head.prev = &temp_head;
    }
}


bool node::mark_reachable(bool update_ref_count) {
    if(reachable) {
        if(update_ref_count)
            ++ref_count;
        return false;
    }

    if(update_ref_count)
        ref_count = 1;
    reachable = true;
    list_remove();
    list_insert(temp_head);
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

    //debug_out("collect: freeing unreachables");
    detail::free_unreachable();
    
    debug_out("collect: still reachable nodes: " + std::to_string(object_count())
            + ", anchors: " + std::to_string(anchor_count()));
}


std::size_t object_count() {
    if(debug && detail::nested_create_count == 0) {
        if(detail::temp_head.next != &detail::temp_head)
            debug_error("temp list is not empty");
        if(detail::temp_head.prev != &detail::temp_head)
            debug_error("temp_head.prev != head");
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


std::size_t get_memory_used() { return detail::memory_used; }


std::size_t get_memory_limit() { return detail::memory_limit; }


void set_memory_limit(std::size_t limit) {
    detail::memory_limit = limit;
    if(detail::memory_used > detail::memory_limit)
        collect();
}


}  // namespace gc

