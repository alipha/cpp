/*
Copyright (c) 2022 Kevin Spinar (Alipha)

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#ifndef LIPH_FUNCTION_TRAITS_HPP
#define LIPH_FUNCTION_TRAITS_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace liph {


template<typename Func>
struct function_traits {
private:
    template<typename Ret, typename... Args>
    static Ret get_return(Ret(*)(Args...));
    
    template<typename Ret, typename... Args>
    static void get_class(Ret(*)(Args...));
    
    template<typename Ret, typename... Args>
    static std::tuple<Args...> get_args(Ret(*)(Args...));
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...));
    
    template<typename Ret, typename Class, typename... Args>
    static Class get_class(Ret(Class::*)(Args...));
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...));
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...) const);
    
    template<typename Ret, typename Class, typename... Args>
    static const Class get_class(Ret(Class::*)(Args...) const);
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...) const);
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...) const &);
    
    template<typename Ret, typename Class, typename... Args>
    static const Class &get_class(Ret(Class::*)(Args...) const &);
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...) const &);
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...) const &&);
    
    template<typename Ret, typename Class, typename... Args>
    static const Class &&get_class(Ret(Class::*)(Args...) const &&);
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...) const &&);
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...) &);
    
    template<typename Ret, typename Class, typename... Args>
    static Class &get_class(Ret(Class::*)(Args...) &);
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...) &);
    
    
    template<typename Ret, typename Class, typename... Args>
    static Ret get_return(Ret(Class::*)(Args...) &&);
    
    template<typename Ret, typename Class, typename... Args>
    static Class &&get_class(Ret(Class::*)(Args...) &&);
    
    template<typename Ret, typename Class, typename... Args>
    static std::tuple<Args...> get_args(Ret(Class::*)(Args...) &&);
    
    
    template<typename Functor>
    static decltype(get_return(&Functor::operator())) get_return(Functor);
    
    template<typename Functor>
    static decltype(get_class(&Functor::operator())) get_class(Functor);
    
    template<typename Functor>
    static decltype(get_args(&Functor::operator())) get_args(Functor);
    
    
public:
    using return_type = decltype(get_return(std::declval<Func>()));
    using class_type = decltype(get_class(std::declval<Func>()));   // note: possibly const-qualified or ref-qualified based upon the if the member function is qualified
    using argument_tuple = decltype(get_args(std::declval<Func>()));
    static constexpr bool is_functor = !std::is_function<Func>::value;
    static constexpr bool is_member = !std::is_same<class_type, void>::value;
    static constexpr bool is_const = std::is_const<typename std::remove_reference<class_type>::type>::value;
    static constexpr bool is_lvalue_ref = std::is_lvalue_reference<class_type>::value;
    static constexpr bool is_rvalue_ref = std::is_rvalue_reference<class_type>::value;
};


template<std::size_t N, typename Func>
struct function_argument_n {
    using type = typename std::tuple_element<N, typename function_traits<Func>::argument_tuple>::type;
};


namespace detail {

template<typename Ret, typename... Args>
Ret (*get_func_ptr(std::tuple<Args...>))(Args...);

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_mem_func_ptr(std::tuple<Args...>))(Args...);

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_const_mem_func_ptr(std::tuple<Args...>))(Args...) const;

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_lvalue_mem_func_ptr(std::tuple<Args...>))(Args...) &;

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_const_lvalue_mem_func_ptr(std::tuple<Args...>))(Args...) const &;

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_rvalue_mem_func_ptr(std::tuple<Args...>))(Args...) &&;

template<typename Ret, typename Class, typename... Args>
Ret (Class::*get_const_rvalue_mem_func_ptr(std::tuple<Args...>))(Args...) const &&;

} // namespace detail


template<typename MemFunc>
struct make_function_pointer {
    using type = decltype(detail::get_func_ptr<typename function_traits<MemFunc>::return_type>(std::declval<typename function_traits<MemFunc>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_member_function_pointer {
    using type = decltype(detail::get_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_const_member_function_pointer {
    using type = decltype(detail::get_const_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_lvalue_member_function_pointer {
    using type = decltype(detail::get_lvalue_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_const_lvalue_member_function_pointer {
    using type = decltype(detail::get_const_lvalue_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_rvalue_member_function_pointer {
    using type = decltype(detail::get_rvalue_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

template<typename Class, typename Func>
struct make_const_rvalue_member_function_pointer {
    using type = decltype(detail::get_const_rvalue_mem_func_ptr<typename function_traits<Func>::return_type, Class>(std::declval<typename function_traits<Func>::argument_tuple>()));
};

}  // namespace liph

#endif

