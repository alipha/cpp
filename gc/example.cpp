#include "gc.hpp"


struct graph_node {
    graph_node(std::string name) : name(std::move(name)) {}

    std::string name;
    std::vector<gc::ptr<graph_node>> connected_nodes;


    // needed by gc library so it knows which objects are connected to which other objects
    void transverse(gc::action &action) {
        action(connected_nodes);
    }
};


int main() {
    // anchor_ptr is used here because we need to anchor "root node" to the program stack
    gc::anchor_ptr<graph_node> root = gc::make_anchor_ptr<graph_node>("root node");
    root->connected_nodes.push_back(gc::make_ptr<graph_node>("child 0"));
    root->connected_nodes.push_back(gc::make_ptr<graph_node>("child 1"));
    root->connected_nodes[0]->connected_nodes.push_back(root);   // make child 0 point back to root
    
    gc::collect(); // nothing happens because "root node", "child 0", and "child 1" are all still 
                   // "anchored to the program stack" via the root anchor_node
    
    root = nullptr;  // doesn't delete anything. "root node" graph_node still exists and 
                     // has a reference count of 1 because "child 0" points to it.
                     // "child 0" and "child 1" have ref counts of 1 because "root node" points to them
    
    gc::collect(); // now, since no graph_node objects are reachable from any anchor_ptrs, delete all of them
}

