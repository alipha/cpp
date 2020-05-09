#include "gc.hpp"

#include <iostream>
#include <set>
#include <map>
#include <unordered_set>
#include <utility>
#include <variant>
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


using variant_obj = std::variant<int, graph_node, std::vector<graph_node>>;
using variant_ptr = std::variant<int, gc::ptr<graph_node>, std::vector<gc::ptr<graph_node>>>;
using variant_anchor = std::variant<int, gc::anchor_ptr<graph_node>, std::vector<gc::anchor_ptr<graph_node>>>;

using tuple_obj = std::tuple<int, graph_node, std::vector<graph_node>>;
using tuple_ptr = std::tuple<int, gc::ptr<graph_node>, std::vector<gc::ptr<graph_node>>>;


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



void create_links(std::vector<gc::anchor_ptr<graph_node>> &anchors, std::vector<std::pair<std::string, std::string>> &links) {
    std::map<std::string, gc::anchor_ptr<graph_node>> node_by_name;

    for(auto &anchor : anchors)
        node_by_name[anchor->name] = anchor;

    for(auto &link : links) {
        auto from_it = node_by_name.find(link.first);
        if(from_it == node_by_name.end())
            from_it = node_by_name.try_emplace(link.first, gc::make_anchor_ptr<graph_node>(link.first)).first;
        
        auto to_it = node_by_name.find(link.second);
        if(to_it == node_by_name.end())
            to_it = node_by_name.try_emplace(link.second, gc::make_anchor_ptr<graph_node>(link.second)).first;

        if(!from_it->second->primary_route)
            from_it->second->primary_route = to_it->second;
        else
            from_it->second->other_routes.push_back(to_it->second);
    }
}



void composition_tests() {
    gc::anchor_ptr<graph_node> one_anchor = gc::make_anchor_ptr<graph_node>("OneAnchor");
    gc::anchor_ptr<graph_node> null_anchor;
    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        gc::ptr<graph_node> unanchored = gc::make_ptr<graph_node>("Unanchored");
        std::cout << (*unanchored).name << std::endl;
        gc::make_ptr<graph_node>("TempPtr");
    }

    gc::make_anchor_ptr<graph_node>("TempAnchor");

    std::vector<gc::anchor_ptr<graph_node>> anchors;
    anchors.emplace_back(std::in_place_t(), "Vector0");
    anchors.emplace_back();
    anchors.push_back(gc::make_ptr<graph_node>("Vector2"));
    anchors[1] = gc::make_ptr<graph_node>("Vector1");

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        gc::ptr<std::vector<graph_node>> ptrs = gc::make_ptr<std::vector<graph_node>>();
        ptrs->emplace_back("UnVector0");
        ptrs->emplace_back("UnVector1");
    }
    std::set<gc::anchor_ptr<graph_node>> anchor_set(anchors.begin(), anchors.end());

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        std::unordered_set<gc::ptr<graph_node>> ptr_unordered_set(anchors.begin(), anchors.end());
    }

    variant_anchor var_int = 3;
    variant_anchor var_an = gc::make_anchor_ptr<graph_node>("VariantAnchor");
    variant_anchor var_ans = std::vector<gc::anchor_ptr<graph_node>>(2, gc::make_anchor_ptr<graph_node>("VariantAnchors"));
    gc::anchor_ptr<variant_ptr> anchor_var = gc::make_anchor_ptr<variant_ptr>(gc::make_ptr<graph_node>("AnchorVariant"));

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        variant_ptr var_ptr_int = 5;
        variant_ptr var_ptr = gc::make_ptr<graph_node>("VariantPtr");
        variant_ptr var_ptrs = std::vector<gc::ptr<graph_node>>(2, gc::make_ptr<graph_node>("VariantPtrs"));
    }

    gc::anchor_ptr<variant_obj> ptr_variant = gc::make_anchor_ptr<variant_obj>(graph_node("PtrVariant"));
    gc::anchor_ptr<variant_obj> ptr_variant2 = gc::make_ptr<variant_obj>(3);

    gc::anchor_ptr<tuple_ptr> anchor_tup = gc::make_ptr<tuple_ptr>(4, 
            gc::make_ptr<graph_node>("AnchorTuple"), 
            std::vector<gc::ptr<graph_node>>(1, gc::make_ptr<graph_node>("AnchorTupleVec0")));
    gc::anchor_ptr<tuple_obj> ptr_tuple = gc::make_anchor_ptr<tuple_obj>(5, graph_node("PtrTuple"), std::vector<graph_node>(1, graph_node("PtrTupleVector0")));
    
    gc::anchor_ptr<graph_node> null_anchor2 = null_anchor;
    gc::ptr<graph_node> null_ptr = null_anchor;     // don't do this

    std::cout << one_anchor->name << std::endl;
    std::cout << anchors[0].get()->name << std::endl;
   

    gc::anchor_ptr<gc::ptr<graph_node>> foo = gc::make_anchor_ptr<gc::ptr<graph_node>>(gc::make_ptr<graph_node>("test"));

    std::cout << "Before collect in composition_tests" << std::endl;
    gc::collect();
}



int main() {
    composition_tests();
    std::cout << "After composition_tests" << std::endl;
    gc::collect();

    //tests();
}

