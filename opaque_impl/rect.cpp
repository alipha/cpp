//
// part of the example and not required by opaque_impl
//
#include "rect.h"

#define self (impl.get<rect_impl>())


struct rect_impl {
    rect_impl(int w, int h) : width(w), height(h) {}
    int width;
    int height;
};

rect::rect(int width, int height) : impl(std::in_place_type_t<rect_impl>(), width, height) {}
    
rect::rect(const rect &other) : impl(other.impl.get<rect_impl>()) {}

rect &rect::operator=(const rect &other) {
    self = other.impl.get<rect_impl>();
    return *this;
}
    
rect::~rect() { self.~rect_impl(); }
    
int rect::area() const { return self.width * self.height; }
int rect::get_width() const { return self.width; }
int rect::get_height() const { return self.height; }
    
void rect::set_width(int w) { self.width = w; }
void rect::set_height(int h) { self.height = h; }

