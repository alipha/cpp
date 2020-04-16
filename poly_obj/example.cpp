#include "poly_obj.h"

#include <iostream>
#include <vector>


struct shape {
    virtual ~shape() = default;
    virtual void print(std::ostream &os) const = 0;
};

struct square : shape {
    square(int size) : size(size) {}
    square(const square &other) noexcept : size(other.size) { std::cout << "square copy ctor" << std::endl; }
    square(square &&other) noexcept : size(other.size) { std::cout << "square move ctor" << std::endl; }
    ~square() { std::cout << "square dtor" << std::endl; }
    
    void print(std::ostream &os) const override { os << "square: size = " << size << std::endl;; }
    int size;
};

struct rectangle : shape {
    rectangle(int width, int height) : width(width), height(height) {}
    rectangle(const rectangle &other) noexcept : width(other.width), height(other.height) { std::cout << "rectangle copy ctor" << std::endl; }
    rectangle(rectangle &&other) noexcept : width(other.width), height(other.height) { std::cout << "rectangle move ctor" << std::endl; }
    ~rectangle() { std::cout << "rectangle dtor" << std::endl; }
    
    void print(std::ostream &os) const override { os << "rectangle: width = " << width << ", height = " << height << std::endl;; }
    int width;
    int height;
};


int main() {
    using shape_obj = liph::poly_obj<shape, liph::max_sizeof<square, rectangle>()>;
        
    std::vector<shape_obj> v;
    v.push_back(square(2));
    v.emplace_back(liph::as_type<rectangle>(), 5, 8);
    v.resize(3);
    std::cout << std::endl;
    
    for(shape_obj &obj : v) {
        if(obj.ptr())
            obj->print(std::cout);
        else
            std::cout << "null" << std::endl;
    }
    
    return 0;
}
