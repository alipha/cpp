//
// part of the example and not required by opaque_impl
//
#ifndef RECT_H
#define RECT_H

#include "opaque_impl.h"

class rect {
public:
    rect(int width, int height);
    
    rect(const rect &);
    rect &operator=(const rect &);
    
    ~rect();
    
    int area() const;
    int get_width() const;
    int get_height() const;
    
    void set_width(int);
    void set_height(int);
    
private:
    opaque_impl<struct rect_impl, sizeof(int) * 2> impl;
};

#endif
