#ifndef LIPH_DERIVED_OBJ_H
#define LIPH_DERIVED_OBJ_H

#include <cstddef>
#include <new>
#include <utility>
#include <stdexcept>

namespace liph {

template<typename Base, std::size_t MaxSize>
struct derived_obj {
    using value_type = Base;
    static constexpr std::size_t max_size = MaxSize;
    
    derived_obj(const derived_obj &) = delete;
    derived_obj &operator=(const derived_obj &) = delete;
    
    template<typename Derived>
    derived_obj(Derived &&derived) {
        assign(std::forward<Derived>(derived));
    }
    
    template<typename Derived>
    derived_obj &operator=(Derived &&derived) {
        get()->~Base();
        return assign(std::forward<Derived>(derived));
    }
    
    template<typename Derived>
    derived_obj &assign(Derived &&derived) {
        static_assert(sizeof(Derived) <= MaxSize, "Derived object is larger than MaxSize bytes");
        using ValueType = std::decay_t<Derived>;
        
        ValueType *p = new (data) ValueType(std::forward<Derived>(derived));
        try { 
            if(static_cast<void*>(static_cast<Base*>(p)) != static_cast<void*>(data))
                throw std::logic_error("Base must be Derived's first base class");
        } catch(...) {
            p->~ValueType();
            throw;
        }
        
        return *this;
    }
    
    const Base &operator*() const { return *get(); }
    Base &operator*() { return *get(); }
    
    const Base *operator->() const { return get(); }
    Base *operator->() { return get(); }
    
    const Base *get() const { return std::launder(reinterpret_cast<Base*>(data)); }
    Base *get() { return std::launder(reinterpret_cast<Base*>(data)); }
    
    ~derived_obj() { get()->~Base(); }
    
private:
    alignas(alignof(std::max_align_t)) char data[MaxSize];
};

} // namespace liph

#endif

