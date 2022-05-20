#ifndef SYSTEM_H
#define SYSTEM_H

#include <functional>
#include <iostream>
#include <unordered_map>

class SystemFactory {
  public:
    static std::unordered_map<std::string, std::function<void()>> &registered() {
        static std::unordered_map<std::string, std::function<void()>> instance =
                std::unordered_map<std::string, std::function<void()>>();
        return instance;
    }
};


template <class T>
static bool register_system() {
    auto name = T().get_name();
    SystemFactory::registered().emplace(
        name, [=]() { std::cout << name << std::endl; });
    return true;
}

template <class T>
class RegisteredSystem {
    template <typename X, X>
    struct force_init {};

    static bool registered;
    typedef force_init<bool &, registered> trick;

  public:
    std::string get_name() { return static_cast<T *>(this)->name; }
};

template <class T>
bool RegisteredSystem<T>::registered = register_system<T>();

#endif
