#ifndef LIPH_CALLBACK_FACTORY_DETAIL_HPP
#define LIPH_CALLBACK_FACTORY_DETAIL_HPP

#include <cstddef>
#include <cstdint>


namespace liph {

class callback_factory;


namespace detail {


struct callback_block {
    unsigned char code[37];
    void (*deleter)(unsigned char*);
};


struct block_ref {
    class callback_block *block_ptr;
    const class callback_page *page_ptr;
};


struct callback_page {

    explicit callback_page(callback_factory *factory);

    callback_page(callback_page &&other);
    callback_page &operator=(callback_page &&other);

    ~callback_page();

    callback_block *allocate() const;

    bool deallocate(callback_block *ptr) const;

    static callback_block *map_page();

    callback_factory *factory;
    callback_block *start;
    mutable callback_block *next_available;
    mutable std::size_t unused_block_count;

    static const int page_size;
    static const int max_block_count;
};


inline bool operator<(const callback_page &left, const callback_page &right) {
    return left.start < right.start;
}

inline bool operator<(const callback_page &left, void (*right)()) {
    return left.start < reinterpret_cast<void*>(right);
}

inline bool operator<(void (*left)(), const callback_page &right) {
    return reinterpret_cast<void*>(left) < right.start;
}


struct callback_deleter {
    // TODO: for unique_ptr and shared_ptr
    const callback_page *page;
};


struct mem_func_ptr {
    std::uint64_t func_ptr_or_vtable_off;
    std::uint64_t obj_offset;
};


}  // namespace detail

}  // namespace liph

#endif
