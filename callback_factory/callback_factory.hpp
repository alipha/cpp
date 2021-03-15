#ifndef LIPH_CALLBACK_FACTORY_HPP
#define LIPH_CALLBACK_FACTORY_HPP

#include "callback_factory_detail.hpp"
#include "function_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>


namespace liph {


struct callback_factory {
    template<typename Func>
    using unique_ptr = std::unique_ptr<Func, detail::callback_deleter>;

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

private:
    friend class detail::callback_page;

    template<typename T, typename Class>
    static detail::callback_block make_block(T &&obj_ptr, void (Class::*mem_ptr)()) {
        if constexpr(std::is_pointer_v<std::remove_reference_t<T>>) {
            using Ptr = std::remove_reference_t<T>;
            static_assert(std::is_convertible_v<Ptr, const Class*>);

            detail::callback_block block{{}, [](unsigned char*){}};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(obj_ptr), reinterpret_cast<detail::mem_func_ptr&>(mem_ptr));
            return block;
        } else {
            using Obj = std::decay_t<T>;
            static_assert(std::is_convertible_v<Obj*, Class*>);

            Obj *copy = new Obj(std::forward<T>(obj_ptr));
            detail::callback_block block{{}, &obj_deleter<Obj, 0>};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(copy), reinterpret_cast<detail::mem_func_ptr&>(mem_ptr));
            return block;
        }
    }

    static void patch_code(unsigned char *code, std::uint64_t obj_ptr, detail::mem_func_ptr func);
    static std::uint64_t resolve_func(std::uint64_t obj_ptr, detail::mem_func_ptr func); 

    detail::block_ref allocate(detail::callback_block &&block);
    detail::callback_block *allocate_space();


    std::set<detail::callback_page, std::less<>> init_pages() {
        std::set<detail::callback_page, std::less<>> ret;
        ret.emplace(this);
        return ret;
    }


    template<typename Class, std::size_t IntArgs>
    static void obj_deleter(unsigned char *code) {
        delete *reinterpret_cast<Class**>(code + 12 + 3 * IntArgs);
    }


    std::set<detail::callback_page, std::less<>> pages;
    std::vector<detail::block_ref> unused_blocks;
    const detail::callback_page *current_page;
};


}  // namespace liph

#endif
