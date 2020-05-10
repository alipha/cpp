#include "gc.hpp"
#include <iostream>


struct shape {
    virtual ~shape() {}
    virtual void draw() const = 0;

    void transverse(gc::action &act) { act(inner); }

    gc::ptr<shape> inner;
};


struct circle : shape {
    circle(int radius) : r(radius) {}

    void draw() const override { 
        std::cout << "circle. radius: " << r << std::endl; 
        if(inner) {
            std::cout << "- inner: ";
            inner->draw();
        }
    }

    int r;
};


struct rectangle : shape {
    rectangle(int width, int height) : w(width), h(height) {}

    void draw() const override { 
        std::cout << "rectangle. " << w << " by " << h << std::endl;
        if(inner != nullptr) {
            std::cout << "- inner: ";
            inner->draw();
        }
    }

    int w;
    int h;
};


int main() {
    gc::anchor_ptr<rectangle> r = gc::make_anchor_ptr<rectangle>(3, 5);
    r->inner = gc::make_ptr<circle>(4);
    r->inner->inner = gc::make_ptr<rectangle>(10, 20);

    gc::anchor_ptr<const shape> p = r;
    while(p > nullptr) {
        p->draw();

        if(gc::dynamic_pointer_cast<const rectangle>(p)) {
            gc::ptr<const rectangle> r2 = gc::static_pointer_cast<const rectangle>(p);
            gc::ptr<rectangle> r3 = gc::const_pointer_cast<rectangle>(r2);
            std::cout << "again: ";
            r3->draw();
        }

        if(gc::anchor_ptr<const circle> c = gc::dynamic_pointer_cast<const circle>(p)) {
            std::cout << "CIRCLE AGAIN: ";
            c->draw();
        }

        p = p->inner;
    }

    
    gc::for_types<rectangle>::iterate_all_objects([](auto &&r) { std::cout << "area: " << r.w * r.h << std::endl; });
   
    auto rect_or_void = [](auto &&r) {
        if constexpr(std::is_same_v<void *, std::remove_reference_t<decltype(r)>>) {
            std::cout << "void*: " << static_cast<circle*>(r)->r << std::endl;
        } else {
            std::cout << "perimeter: " << 2 * (r.w + r.h) << std::endl;
        }
    };

    gc::for_types<rectangle, void*>::iterate_all_objects(rect_or_void);
    gc::for_types<void*, rectangle>::iterate_all_objects(rect_or_void);

    gc::for_types<rectangle, circle>::iterate_all_objects([](auto &&s) {
        using shape_type = std::remove_reference_t<decltype(s)>;
        if constexpr(std::is_same_v<shape_type, rectangle>) {
            std::cout << "is rectangle" << std::endl;
        } else if constexpr(std::is_same_v<shape_type, circle>) {
            std::cout << "is circle" << std::endl;
        } else {
            static_assert(sizeof(s) && false, "not a rectangle or circle");
        }
    });
}
