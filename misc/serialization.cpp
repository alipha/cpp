#include <iostream>
#include <string>
#include <cstdint>
#include <utility>
#include <https://raw.githubusercontent.com/alipha/cpp/master/endian/endian.hpp>


struct User {
    User(std::string n, std::uint32_t a) : name(std::move(n)), age(a) {}
    std::string name;
    std::uint32_t age;
};

/* serialized format (little endian):
    name_length: 4-byte integer;
    name:        "name_length" bytes;
    age:         4-byte integer;
*/

std::string serialize(User user) {
    std::size_t total_size = 4 + user.name.size() + 4;
    std::string bytes(total_size, '\0');

    char *current_pos = bytes.data();

    uint32_to_le(user.name.size(), current_pos);
    current_pos += 4;

    std::copy(user.name.begin(), user.name.end(), current_pos);
    current_pos += user.name.size();

    uint32_to_le(user.age, current_pos);

    return bytes;
}

User deserialize(const std::string &bytes) {
    const char *current_pos = bytes.data();

    std::uint32_t name_length = le_to_uint32(current_pos);
    current_pos += 4;

    User user{std::string(name_length, '\0'), 0};
    std::copy(current_pos, current_pos + name_length, user.name.data());
    current_pos += name_length;

    user.age = le_to_uint32(current_pos);
    return user;
}


void print(const std::string &str) {
    for(char ch : str) {
        if(ch < ' ' || ch > '~') {
            std::cout << '.';       // don't display non-printable characters
        } else {
            std::cout << ch;
        }
    }
    std::cout << '\n';
}


int main() {
    User user("Lorenzo Von Matterhorn the Second", 37);
    std::string bytes = serialize(user);
    print(bytes);

    User user2 = deserialize(bytes);
    std::cout << "Name: '" << user2.name << "'\n";
    std::cout << "Age: " << user2.age << '\n';
}
