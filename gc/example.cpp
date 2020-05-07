#include "gc.hpp"

#include <iostream>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>



struct graph_node {
    graph_node(std::string name) : name(std::forward<std::string>(name)) {}

    std::string name;

    gc::ptr<graph_node> primary_route;
    std::vector<gc::ptr<graph_node>> other_routes;


    ~graph_node() {
        std::cout << "destruct: " << name << std::endl;
    }

    void transverse(gc::action &action) {
        action(primary_route);
        action(other_routes);
    }

    void before_destroy();
};


std::ostream &operator<<(std::ostream &os, const graph_node &node) {
    std::string primary = node.primary_route ? node.primary_route->name : "null";

    os << node.name << " [" << primary << "][";

    for(const gc::ptr<graph_node> &route : node.other_routes) {
        std::string name = route ? route->name : "null";
        os << name << ", ";
    }

    return os << ']';
}


void graph_node::before_destroy() { 
    std::cout << "before ~: " << *this << std::endl;
}


int main() {
    gc::anchor_ptr<graph_node> one_anchor = gc::make_anchor_ptr<graph_node>("OneAnchor");
    gc::anchor_ptr<graph_node> null_anchor;
/*    gc::ptr<graph_node> unanchored = gc::make_ptr<graph_node>("Unanchored");

    gc::make_ptr<graph_node>("TempPtr");
    gc::make_anchor_ptr<graph_node>("TempAnchor");

    std::vector<gc::anchor_ptr<graph_node>> anchors;
    anchors.emplace_back(std::in_place_t(), "Vector0");
    anchors.emplace_back();
    anchors.push_back(gc::make_ptr<graph_node>("Vector2"));
    anchors[1] = gc::make_ptr<graph_node>("Vector1");

    gc::ptr<std::vector<graph_node>> ptrs = gc::make_ptr<std::vector<graph_node>>();
    ptrs->emplace_back("UnVector0");
    ptrs->emplace_back("UnVector1");

    std::set<gc::anchor_ptr<graph_node>> anchor_set(anchors.begin(), anchors.end());
    std::unordered_set<gc::ptr<graph_node>> ptr_unordered_set(anchors.begin(), anchors.end());

    gc::anchor_ptr<graph_node> null_anchor2 = null_anchor;
    gc::ptr<graph_node> null_ptr = null_anchor;

    std::cout << one_anchor->name << std::endl;
    std::cout << (*unanchored).name << std::endl;
    std::cout << anchors[0].get()->name << std::endl;
*/    gc::collect();
}

