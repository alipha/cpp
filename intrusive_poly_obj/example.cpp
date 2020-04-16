#include "intrusive_poly_obj.h"

#include <iostream>
#include <vector>


struct shape {
    virtual ~shape() = default;
    virtual void copy_to(void *dest) const = 0;
    virtual void move_to(void *dest) noexcept = 0;
    
    virtual void print(std::ostream &os) const = 0;
};

struct square : liph::implement_poly_obj<shape, square> {
    square(int size) : size(size) {}
    square(const square &other) noexcept : size(other.size) { std::cout << "square copy ctor" << std::endl; }
    square(square &&other) noexcept : size(other.size) { std::cout << "square move ctor" << std::endl; }
    ~square() { std::cout << "square dtor" << std::endl; }
    
    void print(std::ostream &os) const override { os << "square: size = " << size << std::endl;; }
    int size;
};

struct rectangle : liph::implement_poly_obj<shape, rectangle> {
    rectangle(int width, int height) : width(width), height(height) {}
    rectangle(const rectangle &other) noexcept : width(other.width), height(other.height) { std::cout << "rectangle copy ctor" << std::endl; }
    rectangle(rectangle &&other) noexcept : width(other.width), height(other.height) { std::cout << "rectangle move ctor" << std::endl; }
    ~rectangle() { std::cout << "rectangle dtor" << std::endl; }
    
    void print(std::ostream &os) const override { os << "rectangle: width = " << width << ", height = " << height << std::endl;; }
    int width;
    int height;
};


int main() {
    using shape_obj = liph::intrusive_poly_obj<shape, liph::max_sizeof<square, rectangle>()>;
    
    shape_obj s(liph::as_type<square>(), 22);
    shape_obj r(liph::as_type<rectangle>(), 55, 88);
    shape_obj n(s);
    r = std::move(s);
    
    std::vector<shape_obj> v;
    v.push_back(square(2));
    v.emplace_back(liph::as_type<rectangle>(), 5, 8);
    std::cout << std::endl;
    
    for(shape_obj &obj : v) {
        obj->print(std::cout);
    }
    
    return 0;
}
