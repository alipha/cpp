#ifndef LIPH_CALLBACK_FACTORY_DETAIL_HPP
#define LIPH_CALLBACK_FACTORY_DETAIL_HPP

#include "function_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>


namespace liph {

class callback_factory;


namespace detail {


template<typename T>
constexpr std::size_t register_count() {
	if(std::is_member_function_pointer_v<T>) {
		return 2;
	} else if(std::is_reference_v<T> || std::is_pointer_v<T> || std::is_integral_v<T> || std::is_enum_v<T>) {
		return sizeof(T) == 16 ? 2 : 1;
	} else if(std::is_floating_point_v<T>) {
		return 0;
	} else if(!std::is_trivially_copyable_v<T>) {
		return 1;
	} else if(sizeof(T) <= 8) {
		return 1;
	} else if(sizeof(T) <= 16) {
		return 2;
	} else {
		return 1;
	}
}


template<typename Tuple>
struct registers_used {
	static_assert(sizeof(Tuple) && false, "Template argument must be a std::tuple");
};

template<>
struct registers_used<std::tuple<>> {
	static constexpr std::size_t value = 0;
};

template<typename... Args>
struct registers_used<std::tuple<Args...>> {
	static constexpr std::size_t value = (register_count<Args>() + ...);
};

template<typename Tuple>
constexpr std::size_t registers_used_v = registers_used<Tuple>::value;



template<typename MemFunc, typename Class, typename Tuple>
struct is_mem_func_invocable {
	static_assert(sizeof(Tuple) && false, "3rd template argument must be a std::tuple");
};

template<typename MemFunc, typename Class, typename... Args>
struct is_mem_func_invocable<MemFunc, Class, std::tuple<Args...>> {
    static constexpr bool value = std::is_invocable_v<MemFunc, Class, Args...>;
};

template<typename MemFunc, typename Class, typename Tuple>
static constexpr bool is_mem_func_invocable_v = is_mem_func_invocable<MemFunc, Class, Tuple>::value;


template<typename T>
using object_type = std::remove_pointer_t<std::remove_reference_t<T>>;


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

inline bool operator<(const callback_page &left, std::uint64_t right) {
    return left.start < reinterpret_cast<void*>(right);
}

inline bool operator<(std::uint64_t left, const callback_page &right) {
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
