#ifndef LIPH_CALLBACK_FACTORY_HPP
#define LIPH_CALLBACK_FACTORY_HPP

#include <cstddef>
#include <memory>
#include <set>
#include <vector>


struct callback_block {
    unsigned char code[37];
    void (*deleter)(unsigned char*);
};


struct block_ref {
    class callback_block *block_ptr;
    const class callback_page *page_ptr;
};


struct callback_page {

    explicit callback_page(class callback_factory *factory);

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
    // TODO
    const callback_page *page;
};


struct mem_func_ptr {
    std::uint64_t func_ptr_or_vtable_off;
    std::uint64_t obj_offset;
};


struct callback_factory {
    template<typename Func>
    using unique_ptr = std::unique_ptr<Func, callback_deleter>;

    using func_ptr = void(*)();


    callback_factory() :
        pages{init_pages()},
        unused_blocks(),
        current_page(&*pages.begin()) {}


    template<typename T, typename Class>
    func_ptr make_raw_callback(T &&obj_ptr, void (Class::*mem_ptr)()) {
        auto [block_ptr, page_ptr] = allocate(make_block(std::forward<T>(obj_ptr), mem_ptr));
        return reinterpret_cast<func_ptr>(block_ptr->code);
    }

    void free_raw_callback(func_ptr ptr);


    template<typename T, typename Class>
    static callback_block make_block(T &&obj_ptr, void (Class::*mem_ptr)()) {
        if constexpr(std::is_pointer_v<std::remove_reference_t<T>>) {
            using Ptr = std::remove_reference_t<T>;
            static_assert(std::is_convertible_v<Ptr, const Class*>);

            callback_block block{{}, [](unsigned char*){}};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(obj_ptr), reinterpret_cast<mem_func_ptr&>(mem_ptr));
            return block;
        } else {
            using Obj = std::decay_t<T>;
            static_assert(std::is_convertible_v<Obj*, Class*>);

            Obj *copy = new Obj(std::forward<T>(obj_ptr));
            callback_block block{{}, &obj_deleter<Obj, 0>};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(copy), reinterpret_cast<mem_func_ptr&>(mem_ptr));
            return block;
        }
    }

    static void patch_code(unsigned char *code, std::uint64_t obj_ptr, mem_func_ptr func);
    static std::uint64_t resolve_func(std::uint64_t obj_ptr, mem_func_ptr func); 

    block_ref allocate(callback_block &&block);
    callback_block *allocate_space();


    std::set<callback_page, std::less<>> init_pages() {
        std::set<callback_page, std::less<>> ret;
        ret.emplace(this);
        return ret;
    }


    template<typename Class, std::size_t IntArgs>
    static void obj_deleter(unsigned char *code) {
        delete *reinterpret_cast<Class**>(code + 12 + 3 * IntArgs);
    }


    std::set<callback_page, std::less<>> pages;
    std::vector<block_ref> unused_blocks;
    const callback_page *current_page;
};


#endif
