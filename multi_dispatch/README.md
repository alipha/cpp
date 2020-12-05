# multi_dispatch

## Introduction

This is a library to enable an easy and safe way to perform Multiple Dispatch (e.g., Double Dispatch) at nearly the same performance as the equivalent Visitor Pattern implementation, 
but without cluttering your classes with all the Visitor Pattern boilerplate. It allows you to separate the dynamically-dispatched functions from the class implementations,
so that adding new derived classes or dynamically-dispatched functions doesn't require recompilation of all the derived classes.


## Typical Visitor Pattern Implementation

Here's what a Visitor Pattern implementation of Double Dispatch may look like:

```cpp
class circle;
class square;
class rectangle;

class shape {
public:
    virtual ~shape() {}
        
    // All this boilerplate that needs to be updated whenever a new
    // derived class is added
    virtual bool contains_base(const shape &other) const = 0;
    virtual bool contains_derived(const circle &other) const = 0;
    virtual bool contains_derived(const square &other) const = 0;
    virtual bool contains_derived(const rectangle &other) const = 0;
};

// contains performs Double Dispatch to
// determine if one shape contains another shape
inline bool contains(const shape &outside, const shape &inside) {
    return x.contains_base(y);
}

class circle : public shape {
public:
    circle(double x, double y, double radius) : x(x), y(y), radius(radius) {}
        
    // this function needs to be added in each derived class and is identical
    bool contains_base(const shape &other) const override { return other.contains_derived(*this); }
    
    // need to add a contains_derived for each derived class
    bool contains_derived(const circle &other) const override { /* calculation for determining if a circle contains a circle */ }
    bool contains_derived(const square &other) const override { /* calculation for determining if a circle contains a square */ }
    bool contains_derived(const rectangle &other) const override { /* calculation for determining if a circle contains a rectangle */ }
    
    double x, y, radius;
};

// definitions of square and rectangle omitted, but are very similar to circle
```


## Typical multi_dispatch implementation

I'll outline the required steps here, but look at all the files in the `shapes/` directory and look at `functions/contains.hpp` and `functions/contains.cpp` for the
full implementation. This `contains` function example is the basic usage of the library, while the other examples highlight additional features.

First, multi_dispatch is slightly intrusive, but not as intrusive as the Visitor Pattern. multi_dispatch requires that the base class (superclass) and 
derived classes (subclasses) inherit from "registrar" classes:

```cpp
// shapes/shape.hpp

class shape : public superclass_registrar<shape> {
public:
    virtual ~shape() {}
    // note that no mention of a "contains" function is needed here
};

// shapes/circle.hpp

class circle : public subclass_registrar<shape, circle> {
public:
    circle(double x, double y, double radius) : 
        // if parameters need to be passed to the shape class, subclass_registrar will pass those through, e.g.
        // subclass_registrar<shape, circle>(arg1, arg2, arg3),
        x(x), y(y), radius(radius) {}
};

// definitions of square and rectangle omitted, but are very similar to circle
```

Then we need to specify what subclasses are related to the superclass, as this information is used by subclass_registry and multi_dispatch:

```cpp
// shapes/shapes_fwd.hpp

// you could provide forward class declarations if you wish instead of specifying `class` within subclass_list
using shapes = subclass_list<class shape, std::tuple<class circle, class square, class rectangle>>; 
```

And we need to instantiate subclass_registry for the above shapes. subclass_registry ensures that each derived class is listed in subclass_list and assigns 
unique integer ids to the derived classes in order to perform the lookups for dispatching.

```cpp
// shapes/shape_registry.cpp
template class subclass_registry<shapes>;
```

Then finally, we can create functions which perform multi-dispatch:

```cpp
// functions/contains.hpp
bool contains(const shape &outside, const shape &inside);

// functions/contains.cpp
// now we can group all the contains implementations together instead of being spread out across various class definitions!
// note that inline helps ensure the compiler optimizes out these functions
inline bool contains_impl(const circle &outside, const circle &inside)       { /* calculation for determining if a circle contains a circle */ }
inline bool contains_impl(const circle &outside, const square &inside)       { /* calculation for determining if a circle contains a square */ }
inline bool contains_impl(const circle &outside, const rectangle &inside)    { /* calculation for determining if a circle contains a rectangle */ }
inline bool contains_impl(const square &outside, const circle &inside)       { /* calculation for determining if a square contains a circle */ }
inline bool contains_impl(const square &outside, const square &inside)       { /* calculation for determining if a square contains a square */ }
inline bool contains_impl(const square &outside, const rectangle &inside)    { /* calculation for determining if a square contains a rectangle */ }
inline bool contains_impl(const rectangle &outside, const circle &inside)    { /* calculation for determining if a rectangle contains a circle */ }
inline bool contains_impl(const rectangle &outside, const square &inside)    { /* calculation for determining if a rectangle contains a square */ }
inline bool contains_impl(const rectangle &outside, const rectangle &inside) { /* calculation for determining if a rectangle contains a rectangle */ }

// note that the Visitor Pattern example would have contained the same number (9) of contains_derived functions, but they 
// were omitted for brevity

// the "contains_impl" functions and "contains" functions have different names so that if a "contains_impl" function was 
// forgotten for a particular derived class combination, you'll get a compiler error instead of overload resolution causing 
// "contains" to be executed recursively
bool contains(const shape &outside, const shape &inside) {   
    return multi_dispatch<shapes>([](auto &outside, auto &inside) {
        return contains_impl(outside, inside);              
    }, outside, inside);
}
```

While using multi_dispatch requires a little boilerplate, the boilerplate is easy to copy and modify from this example. Also, this is much better organized!


## Features

* No macros

* No RTTI

* Function calls are dispatched efficiently via lookup tables

* Compiler or linker errors will catch missing implementations

* Multi-dispatched functions can be added without recompiling all the subclasses

* Subclasses can be added without recompiling all other subclasses

* Multi-dispatch can be performed with arguments extended from the same superclass or
    with arguments from different superclasses

* Multi-dispatch can be performed with any number of arguments, including a single argument,
    which is effectively creating a "virtual non-member function"

* If argument order does not matter (e.g., collides(Square&, Circle&) has the same implementation
    as collides(Circle&, Square&)) then unordered_multi_dispatch may be used to avoid having to
    write both overloads (TO BE IMPLEMENTED)

## Performance

Individual objects require no additional space cost. They need to be polymorphic, but they are probably already polymorphic. However, if 
they're not, then storage for a vtable pointer is added to each object.

The generated assembly for performing the multi-dispatch is what I would expect when compiled with `-O2`. A virtual call is made for 
each object in order to get their derived class IDs. Multiplication and addition are performed on those IDs to compute the index in
a function call lookup table, which that function is then called.

A Visitor Pattern implementation would only contain a virtual call for each object and would not involve the additional lookup table
arithmetic and additional function call.

## Limitations

* subclass_registry performs initialization work prior main executed, and so multi-dispatch may not work correctly if attempted prior main starting.
