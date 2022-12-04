#include "enum.h"
#include <iostream>


enum class food { apple = 2, banana = 5, carrot = 6 };

int main() {    
     bool x = is_enum_value<food, food(1)>();
     bool y = is_enum_value<food, food(2)>();

    // only counts enum values between -127 and 127 by default
    std::size_t c = count_enum_values<food>();
    constexpr std::array<food, 3> values = enum_values<food>();    // type is: std::array<food, 3>

    constexpr std::string_view apple = enum_value_name<food, food(2)>();
    constexpr std::string_view four = enum_value_name<food, food(4)>();

    std::cout << x << y << std::endl;
    std::cout << c << std::endl << std::endl;

    for(food val : values)
        std::cout << int(val) << std::endl;
    std::cout << apple << std::endl;
    std::cout << four << std::endl;

    constexpr auto names = enum_value_names<food>();  // type is: std::array<std::string_view, 3>
    for(std::string_view name : names)
        std::cout << name << std::endl;

    constexpr auto value_name_pairs = enum_value_mapping<food>();  // type is: std::array<std::pair<food, std::string_view>, 3>
    for(std::pair<food, std::string_view> val : value_name_pairs)
        std::cout << int(val.first) << " = " << val.second << std::endl;

    std::map<food, std::string_view> m = enum_map<food>();
    for(std::pair<food, std::string_view> val : m)
        std::cout << int(val.first) << " = " << val.second << std::endl;
}
