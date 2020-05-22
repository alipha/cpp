#include "gc.hpp"

#include <iostream>
#include <set>
#include <map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>



struct graph_node {
    graph_node() : name("<default>") {}
    graph_node(std::string name) : name(std::forward<std::string>(name)) {}

    graph_node(std::string name, std::vector<std::string> others) : name(std::forward<std::string>(name)) {
        for(auto &other_name : others)
            other_routes.push_back(gc::make_ptr<graph_node>(std::move(other_name)));
    }

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


bool operator<(const graph_node &left, const graph_node &right) { return left.name < right.name; }


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
    gc::anchor<std::map<std::string, gc::ptr<graph_node>>> node_by_name;

    for(auto &anchor : anchors)
        (*node_by_name)[anchor->name] = anchor;

    for(auto &link : links) {
        auto from_it = node_by_name->find(link.first);
        if(from_it == node_by_name->end())
            from_it = node_by_name->try_emplace(link.first, gc::make_ptr<graph_node>(link.first)).first;
        
        auto to_it = node_by_name->find(link.second);
        if(to_it == node_by_name->end())
            to_it = node_by_name->try_emplace(link.second, gc::make_ptr<graph_node>(link.second)).first;

        if(!from_it->second->primary_route)
            from_it->second->primary_route = to_it->second;
        else
            from_it->second->other_routes.push_back(to_it->second);
    }
}



void composition_tests() {
    gc::anchor<graph_node> one_anchor = gc::make_anchor<graph_node>("OneAnchor");
    gc::anchor<graph_node> two_anchor(graph_node("TwoAnchor"));
    two_anchor = one_anchor;
    two_anchor = graph_node("TwoAnchor2");

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
        
    gc::anchor<std::vector<graph_node>> anchors2;
    anchors2->emplace_back("2Vector0");
    anchors2->emplace_back();
    anchors2->push_back(graph_node("2Vector2"));
    anchors2.get()[1] = graph_node("2Vector1");

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        gc::ptr<std::vector<graph_node>> ptrs = gc::make_ptr<std::vector<graph_node>>();
        ptrs->emplace_back("UnVector0");
        ptrs->emplace_back("UnVector1");
    }
    std::set<gc::anchor_ptr<graph_node>> anchor_set(anchors.begin(), anchors.end());
    //gc::anchor<std::set<graph_node>> anchor_set2(std::in_place_t(), anchors2->begin(), anchors2->end());

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        std::unordered_set<gc::ptr<graph_node>> ptr_unordered_set(anchors.begin(), anchors.end());
    }

    variant_anchor var_int = 3;
    variant_anchor var_an = gc::make_anchor_ptr<graph_node>("VariantAnchor");
    variant_anchor var_ans = std::vector<gc::anchor_ptr<graph_node>>(2, gc::make_anchor_ptr<graph_node>("VariantAnchors"));
    gc::anchor_ptr<variant_ptr> anchor_var = gc::make_anchor_ptr<variant_ptr>(gc::make_ptr<graph_node>("AnchorVariant"));
    gc::anchor<variant_ptr> anchor_var2 = gc::make_anchor<variant_ptr>(gc::make_ptr<graph_node>("AnchorVariant"));

    {
        // don't do this. only use gc::anchor_ptr for local variables. this is just a cheap way to test cleanup behavior
        variant_ptr var_ptr_int = 5;
        variant_ptr var_ptr = gc::make_ptr<graph_node>("VariantPtr");
        variant_ptr var_ptrs = std::vector<gc::ptr<graph_node>>(2, gc::make_ptr<graph_node>("VariantPtrs"));
    }

    gc::anchor_ptr<variant_obj> ptr_variant = gc::make_anchor_ptr<variant_obj>(graph_node("PtrVariant"));
    gc::anchor_ptr<variant_obj> ptr_variant2 = gc::make_ptr<variant_obj>(3);
    gc::anchor<variant_obj> anchor_variant(graph_node("PtrVariant"));
    gc::anchor<variant_obj> anchor_variant2(3);

    gc::anchor_ptr<tuple_ptr> anchor_tup = gc::make_ptr<tuple_ptr>(4, 
            gc::make_ptr<graph_node>("AnchorTuple"), 
            std::vector<gc::ptr<graph_node>>(1, gc::make_ptr<graph_node>("AnchorTupleVec0")));
    gc::anchor_ptr<tuple_obj> ptr_tuple = gc::make_anchor_ptr<tuple_obj>(5, graph_node("PtrTuple"), std::vector<graph_node>(1, graph_node("PtrTupleVector0")));
    
    gc::anchor_ptr<graph_node> null_anchor2 = null_anchor;
    gc::ptr<graph_node> null_ptr = null_anchor;     // don't do this

    std::cout << one_anchor->name << std::endl;
    std::cout << anchors[0].get()->name << std::endl;
   

    gc::anchor_ptr<gc::ptr<graph_node>> foo = gc::make_anchor_ptr<gc::ptr<graph_node>>(gc::make_ptr<graph_node>("test"));
    gc::anchor<gc::ptr<graph_node>> foo2 = gc::ptr<graph_node>(gc::make_ptr<graph_node>("test"));

    std::cout << "Before collect in composition_tests" << std::endl;
    gc::collect();
}



void tests() {
    std::vector<gc::anchor_ptr<graph_node>> anchors{
        gc::make_ptr<graph_node>("anchor1"), 
        gc::make_ptr<graph_node>("anchor2")
    };

    std::vector<std::pair<std::string, std::string>> links{
        {"anchor1", "bob1"},
        {"bob1", "steve1"},
        {"bob1", "fred1"},
        {"bob1", "paul1"},
        {"steve1", "fred1"},    // steve1 dies
        {"chris1", "fred1"},    // chris1 dies
        {"xany", "steve1"},
        {"xeuss", "xavier"},
        {"xavier", "xeuss"},
        {"xil", "xray"},
        {"anchor1", "test2"},
        {"anchor2", "test2"},   // test2 dies
        {"anchor2", "foo2"},
        {"anchor2", "moo2"},    // moo2 dies
        {"moo2", "paul1"},
        {"paul1", "foo2"},
        {"anchor2", "meow2"},
        {"anchor2", "woof2"},
        {"meow2", "woof2"},
        {"woof2", "meow2"},
        {"anchor2", "double2"},
        {"anchor2", "double2"}
    };

    create_links(anchors, links);
    std::cout << "created links" << std::endl;

    gc::collect();
    std::cout << "collected" << std::endl;

    anchors.pop_back();
    std::cout << "popped back" << std::endl;

    std::size_t used = gc::get_memory_used();
    gc::set_memory_limit(used + gc::detail::get_memory_used_for<graph_node>() * 3);
    gc::anchor_ptr<graph_node> overflows = gc::make_ptr<graph_node>("overflow", std::vector<std::string>{"overflow2", "overflow3", "overflow4", "overflow5"});
    std::cout << "after memory limit" << std::endl;

    gc::collect();
    std::cout << "collected again" << std::endl;


}



int main() {
    composition_tests();
    std::cout << "After composition_tests" << std::endl;
    gc::collect();

    tests();
    std::cout << gc::get_memory_used() << std::endl;
}

