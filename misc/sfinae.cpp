// First, just a quick example of using non-typename template parameters:

template<typename T, void* VP> void f(T x) {}
template<typename T, bool B>   void g(T x) {}
template<typename T, int N>    void h(T x) {}

int main() {
    f<int, nullptr>(3);
    g<int, false>(4);
    h<int, 42>(5);
}

// Though because the second template parameter can't be inferred from the
// function parameters, we have to specify the template arguments in the
// above example when we call the functions.
// e.g., `f<int, nullptr>(3);` instead of `f(3);`
// So let's default the second argument so we don't have to explicitly specify
// the template arguments:

template<typename T, void* VP = nullptr> void f(T x) {}
template<typename T, bool B = true>      void g(T x) {}
template<typename T, int N = 0>          void h(T x) {}

int main() {
    f(3);   // VP == nullptr
    g(4);   // B == true
    h(5);   // N == 0
}

// Note that we don't actually use VP, B, or N, so we don't actually need to
// name those parameters:

template<typename T, void* = nullptr> void f(T x) {}
template<typename T, bool = true>     void g(T x) {}
template<typename T, int = 0>         void h(T x) {}

int main() {
    f(3);   // second template argument is nullptr
    g(4);   // second template argument is true
    h(5);   // second template argument is 0
}

// Now let's rewrite the above using std::enable_if_t instead:

#include <type_traits>
template<typename T, std::enable_if_t<true, void*> = nullptr> void f(T x) {}
template<typename T, std::enable_if_t<true, bool> = true>     void g(T x) {}
template<typename T, std::enable_if_t<true, int> = 0>         void h(T x) {}

int main() {
    f(3);   // `std::enable_if_t<true, void*> = nullptr` is the same as `void* = nullptr`
    g(4);   // `std::enable_if_t<true, bool> = true`     is the same as `bool = true`
    h(5);   // `std::enable_if_t<true, int> = 0`         is the same as `int = 0`
}


// DETOUR:
// Note that:
template<typename T, std::enable_if_t<true, void*> = nullptr> void f(T x) {}
// can be written as:
template<typename T, std::enable_if_t<true, void>* = nullptr> void f(T x) {}
// since `std::enable_if_t<true, void>` is the same as `void`

// And since std::enable_if_t's second template parameter already defaults to void, we
// can rewrite:
template<typename T, std::enable_if_t<true, void>* = nullptr> void f(T x) {}
// as:
template<typename T, std::enable_if_t<true>* = nullptr> void f(T x) {}
// This is shorter than explicitly specifying void* as the second parameter, which is
// why you end up seeing this form.
// END_OF_DETOUR


// If the std::enable_if_t condition is false, then the std::enable_if_t expression doesn't
// even compile:

#include <type_traits>
template<typename T, std::enable_if_t<false>* = nullptr>   void f(T x) {}
template<typename T, std::enable_if_t<false, bool> = true> void g(T x) {}
template<typename T, std::enable_if_t<false, int> = 0>     void h(T x) {}

// These 3 function declarations all produce errors similar to:
//
// type_traits:2610:11: error: no type named 'type' in 'struct std::enable_if<false, void>'
//      using enable_if_t = typename enable_if<_Cond, _Tp>::type;
//
// Which is the basis of SFINAE (Substitution Failure Is Not An Error):
// - if the std::enable_if_t condition is true, there is no error.
// - if the std::enable_if_t condition is false, then the error will be ignored.
//
// However, the above functions produce errors because there is no attempted
// substitution being done: the condition `false` is definitely false, so the compiler
// evaluates the expression `std::enable_if_t<false, bool>` right away, producing the error.
//
// For SFINAE to occur, the condition has to be dependent upon a template parameter, so that
// the condition's evaulation will be delayed until the stage where the compiler is trying
// to determine what to substitute in for the template parameters: 

#include <type_traits>
template<typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>   void f(T x) {}
template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true> void g(T x) {}
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>     void h(T x) {}

int main() {
    f(3);    // std::is_integral_v<int> is true, so `std::is_integral_v<T>>*` is void*, which compiles
    g(4L);   // std::is_integral_v<long> is true, so `std::enable_if_t<std::is_integral_v<T>, bool>` is bool, which compiles
    h(5.2);  // std::is_integral_v<double> is false, so `std::enable_if_t<std::is_integral_v<T>, int>` produces a error
}

// In the above, the compiler can determine what T is based upon the value passed into the
// the function for the x parameter. So then it plugs that T type into the std::enable_if_t expression.

// Note that `h(5.2);` produces the error:
//
// error: no matching function for call to 'h(double)'
//
// This is because the compiler tried substituting double in for T, which caused the
// std::enable_if_t expression to be invalid. However, SFINAE claims that
// substitution failure is NOT an error, which it isn't--the compiler tried substituting double in
// for T which made the std::enable_if_t expression invalid. But if we provide another overload
// for h which the substitution is valid, then our code would work:

#include <type_traits>
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>       void h(T x) {}
template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0> void h(T x) {}

int main() {
    h(5.2);     // std::is_floating_point_v<T> is true, so the second overload of h is called.
}

// `std::is_integral_v<double>` is false, which causes the first std::enable_if_t to produce
// an error, which means the compiler will ignore this potential substitution.
// Then the compiler tries substituting double into T for the second h function overload, which
// std::is_floating_point_v<T> is true, so the second overload of h is called.

