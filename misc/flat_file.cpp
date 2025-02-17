#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <cstddef>
#include <tuple>


template<typename MemPtr, typename SerializeFunc, typename DeserializeFunc>
class field_def {
public:
    field_def(MemPtr p) 
        : ptr(p), width(default_width_v<MemPtr>), 
        serialize(default_serialize_v<MemPtr>), 
        deserialize(default_deserialize_v<MemPtr>) 
    {}

    field_def(MemPtr p, std::size_t field_width) 
        : ptr(p), width(field_width), 
        serialize(default_serialize_v<MemPtr>), 
        deserialize(default_deserialize_v<MemPtr>)  {}

    field_def(MemPtr p, std::size_t field_width, SerializeFunc s) 
        : ptr(p), width(field_width), serialize(std::move(s)), deserialize(default_deserialize_v<MemPtr>)  {}

    field_def(MemPtr p, std::size_t field_width, SerializeFunc s, DeserializeFunc d) 
        : ptr(p), width(field_width), serialize(std::move(s)), deserialize(std::move(d)) {}

private:
    MemPtr ptr;
    std::size_t width;
    SerializeFunc serialize;
    DeserializeFunc deserialize;
};

template<typename RecordType, typename... FieldDefs>
class record_definition {
public:
private:
    std::tuple<FieldDefs...> fields;
};

template<typename RecordType, typename... FieldDefs>
auto record_def(FieldDefs... defs) {
    return record_definition<RecordType, FieldDefs...>(std::move(defs)...);
}


template<typename RecordType, typename RecordDef>
class flat_file {
public:
    flat_file(const std::string &filename) : file(filename) {}

    RecordType read() {}
    RecordType read_at(std::size_t i) {}

    void write(const RecordType &record) {}
    void write_at(std::size_t i, const RecordType &record) {}

    void resize(std::size_t size) {}

    std::size_t tell() const {}
    std::size_t count() const {}

private:
    std::fstream file;
    RecordDef def;
    std::size_t record_count;
};

struct person {
    std::string first_name;
    char middle_initial;
    std::string last_name;
    int age;
    double gpa;

    auto record_definition() const {
        return record_def<person>(
            field_def(&person::first_name, 16),
            &person::middle_initial,
            field_def(&person::last_name, 16),
            field_def(&person::age, 3),
            field_def(&person::gpa, 5, [](auto gpa, auto size) { std::stringstream ss; ss << std::fixed << std::setprecision(3) << gpa; return ss.str(); })
        );
    }
};



