#ifndef LIPH_ENUM_H
#define LIPH_ENUM_H

#include <array>
#include <string_view>
#include <type_traits>
#include <limits>
#include <utility>
#include <map>


template<auto Value>
struct value_holder {
    static constexpr decltype(Value) value = Value;
};


template<auto Min, typename Func, auto... Indexes>
constexpr void for_range(Func func, std::integer_sequence<long, Indexes...>) {
    (func(value_holder<Indexes+Min>{}), ...);
}

template<auto Min, auto Max, typename Func>
constexpr void for_range(Func func) {
    for_range<Min>(func, std::make_integer_sequence<long, Max-Min+1>());
}


template<typename Enum, Enum value>
constexpr bool is_enum_value() {
    return std::string_view(__PRETTY_FUNCTION__).find("= (") == std::string_view::npos;
}

template<typename Enum, Enum value>
constexpr std::string_view enum_value_name() {
    std::string_view name = __PRETTY_FUNCTION__;
    name = name.substr(0, name.rfind(';'));
    name = name.substr(name.find('('));

    std::size_t before_index = name.rfind(':');

    if(before_index == std::string_view::npos)
        before_index = name.rfind(' ');

    std::size_t paren_index = name.rfind(')');
    if(paren_index > before_index)
        before_index = paren_index;
    return name.substr(before_index + 1, name.length() - before_index);
}


template<typename Enum, 
        std::underlying_type_t<Enum> Min = std::is_signed_v<std::underlying_type_t<Enum>> ? -127 : 0, 
        std::underlying_type_t<Enum> Max = 127>
constexpr std::size_t count_enum_values() {
    std::size_t count = 0;
    for_range<Min, Max>([&](auto i) { 
        count += is_enum_value<Enum, static_cast<Enum>(i.value)>(); 
    });
    return count;
}

template<typename Enum, 
        std::underlying_type_t<Enum> Min = std::is_signed_v<std::underlying_type_t<Enum>> ? -127 : 0, 
        std::underlying_type_t<Enum> Max = 127>
constexpr auto enum_values() {
    std::array<Enum, count_enum_values<Enum, Min, Max>()> values{};
    int index = 0;

    for_range<Min, Max>([&](auto i) { 
        if(is_enum_value<Enum, static_cast<Enum>(i.value)>()) {
            values[index] = static_cast<Enum>(i.value); 
            ++index;
        }
    });
    return values;
}

template<typename Enum, 
        std::underlying_type_t<Enum> Min = std::is_signed_v<std::underlying_type_t<Enum>> ? -127 : 0, 
        std::underlying_type_t<Enum> Max = 127>
constexpr auto enum_value_names() {
    std::array<std::string_view, count_enum_values<Enum, Min, Max>()> values{};
    int index = 0;

    for_range<Min, Max>([&](auto i) { 
        if(is_enum_value<Enum, static_cast<Enum>(i.value)>()) {
            values[index] = enum_value_name<Enum, static_cast<Enum>(i.value)>(); 
            ++index;
        }
    });
    return values;
}

template<typename Enum, 
        std::underlying_type_t<Enum> Min = std::is_signed_v<std::underlying_type_t<Enum>> ? -127 : 0, 
        std::underlying_type_t<Enum> Max = 127>
constexpr auto enum_value_mapping() {
    std::array<std::pair<Enum, std::string_view>, count_enum_values<Enum, Min, Max>()> values{};
    int index = 0;

    for_range<Min, Max>([&](auto i) { 
        constexpr auto value = static_cast<Enum>(i.value);
        if(is_enum_value<Enum, value>()) {
            values[index] = std::pair(value, enum_value_name<Enum, value>()); 
            ++index;
        }
    });
    return values;
}

template<typename Enum, 
        std::underlying_type_t<Enum> Min = std::is_signed_v<std::underlying_type_t<Enum>> ? -127 : 0, 
        std::underlying_type_t<Enum> Max = 127>
auto enum_map() {
    auto pairs = enum_value_mapping<Enum, Min, Max>();
    return std::map(pairs.begin(), pairs.end());
}

#endif
