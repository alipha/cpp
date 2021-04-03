#ifndef LIPH_CALLBACK_FACTORY_HPP
#define LIPH_CALLBACK_FACTORY_HPP

#include "callback_factory_detail.hpp"
#include "function_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>


namespace liph {


struct callback_factory {
    template<typename Func>
    using unique_ptr = std::unique_ptr<Func, detail::callback_deleter>;


    callback_factory() :
        pages{init_pages()},
        unused_blocks(),
        current_page(&*pages.begin()) {}


    template<typename T, typename MemFunc>
    auto make_raw_callback(T &&obj_ptr, MemFunc func) {
        static_assert(is_member_function<MemFunc>);
        static_assert(detail::is_mem_func_invocable_v<MemFunc, T, function_argument_tuple<MemFunc>>,
                "object cannot be used to call member function (is the constness correct?)");

        using NonMemFunc = make_function_pointer<MemFunc>;

        auto [block_ptr, page_ptr] = allocate(make_block(std::forward<T>(obj_ptr), func));
        return reinterpret_cast<NonMemFunc>(block_ptr->code);
    }

    template<typename Lambda>
    auto make_raw_callback(Lambda &&lambda) {
        using T = detail::object_type<Lambda>;
        return make_raw_callback(std::forward<Lambda>(lambda), &T::operator());
    }


    template<typename Func, typename T, typename = std::enable_if_t<!std::is_const_v<detail::object_type<T>>>*>
    auto make_raw_callback_as(T &&obj_ptr, make_member_function_pointer<detail::object_type<T>, Func*> func_ptr) {
        return make_raw_callback(std::forward<T>(obj_ptr), func_ptr);
    }

    template<typename Func, typename T>
    auto make_raw_callback_as(T &&obj_ptr, make_const_member_function_pointer<detail::object_type<T>, Func*> func_ptr) {
        return make_raw_callback(std::forward<T>(obj_ptr), func_ptr);
    }

    template<typename Func, typename Lambda>
    auto make_raw_callback_as(Lambda &&lambda) {
        using T = detail::object_type<Lambda>;
        return make_raw_callback_as<Func>(std::forward<Lambda>(lambda), &T::operator());
    }


    template<typename Func>
    void free_raw_callback(Func *ptr) { 
        static_assert(std::is_function_v<Func>);
        free_raw_callback_impl(reinterpret_cast<std::uint64_t>(ptr));
    }

private:
    friend class detail::callback_page;

    template<typename T, typename MemFunc>
    static detail::callback_block make_block(T &&obj_ptr, MemFunc mem_ptr) {
        using Class = function_class_type<MemFunc>;
        constexpr std::size_t regs_used = detail::registers_used_v<function_argument_tuple<MemFunc>>;

        static_assert(regs_used <= 5, "callback_factory does not support creating callbacks with that many arguments");

        if constexpr(std::is_pointer_v<std::remove_reference_t<T>>) {
            using Ptr = std::remove_reference_t<T>;
            static_assert(std::is_convertible_v<Ptr, const Class*>);

            detail::callback_block block{{}, [](unsigned char*){}};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(obj_ptr), reinterpret_cast<detail::mem_func_ptr&>(mem_ptr), regs_used);
            return block;
        } else {
            using Obj = std::decay_t<T>;
            static_assert(std::is_convertible_v<Obj*, Class*>);

            Obj *copy = new Obj(std::forward<T>(obj_ptr));
            detail::callback_block block{{}, &obj_deleter<Obj, regs_used>};
            patch_code(block.code, reinterpret_cast<std::uint64_t>(copy), reinterpret_cast<detail::mem_func_ptr&>(mem_ptr), regs_used);
            return block;
        }
    }

    void free_raw_callback_impl(std::uint64_t ptr);

    static void patch_code(unsigned char *code, std::uint64_t obj_ptr, detail::mem_func_ptr func, std::size_t regs_used);
    static std::uint64_t resolve_func(std::uint64_t obj_ptr, detail::mem_func_ptr func); 

    detail::block_ref allocate(detail::callback_block &&block);
    detail::callback_block *allocate_space();


    std::set<detail::callback_page, std::less<>> init_pages() {
        std::set<detail::callback_page, std::less<>> ret;
        ret.emplace(this);
        return ret;
    }


    template<typename Class, std::size_t RegsUsed>
    static void obj_deleter(unsigned char *code) {
        static_assert(RegsUsed <= 5);
        delete *reinterpret_cast<Class**>(code + 12 + 3 * RegsUsed);
    }


    std::set<detail::callback_page, std::less<>> pages;
    std::vector<detail::block_ref> unused_blocks;
    const detail::callback_page *current_page;
};


}  // namespace liph

#endif
