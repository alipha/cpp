#ifndef LIPH_FUNCTION_TRAITS_HPP
#define LIPH_FUNCTION_TRAITS_HPP

#include <tuple>
#include <type_traits>

namespace liph {


template<auto Func>
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
    
public:
    using return_type = decltype(get_return(Func));
    using class_type = decltype(get_class(Func));
    using argument_tuple = decltype(get_args(Func));
    static constexpr bool is_member = !std::is_same_v<class_type, void>;
    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<class_type>>;
    static constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<class_type>;
    static constexpr bool is_rvalue_ref = std::is_rvalue_reference_v<class_type>;
};


template<auto Func>
using function_return_type = typename function_traits<Func>::return_type;

template<auto Func>
using function_class_type = typename function_traits<Func>::class_type;

template<auto Func>
using function_argument_tuple = typename function_traits<Func>::argument_tuple;

template<auto Func>
constexpr bool is_member_function = function_traits<Func>::is_member;

template<auto Func>
constexpr bool is_const_member_function = function_traits<Func>::is_const;

template<auto Func>
constexpr bool is_lvalue_member_function = function_traits<Func>::is_lvalue_ref;

template<auto Func>
constexpr bool is_rvalue_member_function = function_traits<Func>::is_rvalue_ref;


}  // namespace liph

#endif

