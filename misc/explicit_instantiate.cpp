#include <string>
#include <cstdint>
#include <tuple>
#include <vector>

class C {
public:
    template<typename T> void f0(std::string const &, T const &);
    template<typename T> void f1(std::string const &, T const &, std::vector<std::size_t> const &);
    template<typename T> void f2(std::string const &, T const &, std::string const &);
    template<typename T> void f3(std::string const &, T const &, std::string const &, std::vector<std::size_t> const &);
};

// just to avoid some repetition of, e.g., `&C::f0<T>` (see below comment)
template<auto Value>
struct value_holder {
	decltype(Value) value = Value;
};

template<typename T>
struct InstantiateHelper {
    value_holder<&C::f0<T>> f0{}; 	// could instead: static constexpr decltype(&C::f0<T>) f0 = &C::f0<T>;
	value_holder<&C::f1<T>> f1{}; 
};

template<typename T>
struct InstantiateScalarHelper {
    value_holder<&C::f0<T>> f0{};
	value_holder<&C::f1<T>> f1{}; 
	value_holder<&C::f2<T>> f2{}; 
	value_holder<&C::f3<T>> f3{}; 
};

template<typename... T>
struct Instantiate {
	static constexpr std::tuple<InstantiateScalarHelper<T>...>                     h1 = {};
	static constexpr std::tuple<InstantiateHelper<std::vector<T>>...>              h2 = {};
	static constexpr std::tuple<InstantiateHelper<std::vector<std::vector<T>>>...> h3 = {};
};

template struct Instantiate<
    float,
    double,
    std::int8_t,
    std::int16_t,
    std::int32_t,
    std::int64_t,
    std::uint8_t,
    std::uint16_t,
    std::uint32_t,
    std::uint64_t
>;
