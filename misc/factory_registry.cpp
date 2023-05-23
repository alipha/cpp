#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>


struct Fruit {
    virtual ~Fruit() = default;
    virtual void print() const = 0;
    virtual std::string get_name() const = 0;
};


template <typename Derived>
class FruitRegistry : public Fruit {
    static bool dummy;

    template <typename T, T>
    struct dummy_value {};
    typedef dummy_value<bool &, dummy>
        dummy_value_type;  // force dummy to get evaluated*/

public:
    std::string get_name() const override { return Derived::name; }
};

class FruitFactory {
    static std::unordered_map<std::string, std::unique_ptr<Fruit> (*)()>
        &get_factories() {
        static std::unordered_map<std::string, std::unique_ptr<Fruit> (*)()>
            derivedFactories;
        return derivedFactories;
    }

    FruitFactory() {}

public:
    template <typename Derived>
    static bool registerType() {
        auto &factories = get_factories();
        factories.try_emplace(Derived::name, []() {
            return std::unique_ptr<Fruit>(new Derived);
        });
        return true;
    }

    static std::unique_ptr<Fruit> create(const std::string &name) {
        auto &factories = get_factories();
        auto it = factories.find(name);
        if (it != factories.end())
            return it->second();
        else
            return std::unique_ptr<Fruit>();  // invalid request
    }

    static std::vector<std::unique_ptr<Fruit>> create_all() {
        auto& factories = get_factories();
        std::vector<std::unique_ptr<Fruit>> result;
        result.reserve(factories.size());
        for (auto& fruit : get_factories()) {
            result.push_back(fruit.second());
        }
        return result;
    }
};

template <typename Derived>
bool FruitRegistry<Derived>::dummy = FruitFactory::registerType<Derived>();



struct Apple : FruitRegistry<Apple> {
    static constexpr const char* name = "apple";
    void print() const override { std::cout << "I'm an apple\n"; }
};

struct Banana : FruitRegistry<Banana> {
    static constexpr const char* name = "banana";
    void print() const override { std::cout << "I'm a banana\n"; }
};

struct Cantaloupe : FruitRegistry<Cantaloupe> {
    static constexpr const char* name = "cantaloupe";
    void print() const override { std::cout << "I'm a cantaloupe\n"; }
};


int main() {
    std::unique_ptr<Fruit> b = FruitFactory::create("banana");
    b->print();
}

