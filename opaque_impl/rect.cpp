//
// part of the example and not required by opaque_impl
//
#include "rect.h"

struct rect_impl {
    rect_impl(int w, int h) : width(w), height(h) {}
    int width;
    int height;
};

rect::rect(int width, int height) : impl(width, height) {}
    
rect::rect(const rect &other) = default;
rect &rect::operator=(const rect &other) = default;
    
rect::~rect() = default;
    
int rect::area() const { return impl->width * impl->height; }
int rect::get_width() const { return impl->width; }
int rect::get_height() const { return impl->height; }
    
void rect::set_width(int w) { impl->width = w; }
void rect::set_height(int h) { impl->height = h; }
