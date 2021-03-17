#ifndef LIPH_C_CALLBACK_HPP
#define LIPH_C_CALLBACK_HPP

#include "function_traits.hpp"
#include <tuple>
#include <type_traits>
#include <utility>

namespace liph {

namespace detail {

template<int N, typename Func, typename T, typename... Args>
auto make_c_callback_impl(T *obj_ptr, Func func_ptr, std::tuple<Args...>*) {
	static T *op;
	static decltype(func_ptr) fp;
	
	op = obj_ptr;
	fp = func_ptr;
	
	return [](Args... args) -> function_return_type<Func> { return (op->*fp)(std::forward<Args>(args)...); };
}

} // namespace detail

	
template<int N = 0, typename Func, typename T>
auto make_c_callback(T *obj_ptr, Func func_ptr) {
	return detail::make_c_callback_impl<N>(obj_ptr, func_ptr, static_cast<function_argument_tuple<Func>*>(nullptr));
}


template<typename Func, int N = 0, typename T>
Func *make_c_callback_as(T *obj_ptr, make_member_function_pointer<T, Func*> func_ptr) {
	return make_c_callback<N>(obj_ptr, func_ptr);
}

template<typename Func, int N = 0, typename T>
Func *make_c_callback_as(T *obj_ptr, make_const_member_function_pointer<T, Func*> func_ptr) {
	return make_c_callback<N>(obj_ptr, func_ptr);
}


template<typename Lambda>
auto make_c_callback(Lambda &&lambda) {
	static std::remove_reference_t<Lambda> lamb{lambda};
		
	return make_c_callback(&lamb, &decltype(lamb)::operator());
}


template<typename Func, typename Lambda>
Func *make_c_callback_as(Lambda &&lambda) {
	static std::remove_reference_t<Lambda> lamb{lambda};
		
	return make_c_callback_as<Func>(&lamb, &decltype(lamb)::operator());
}

} // namespace liph

#endif
