#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

namespace srd::record {

enum class FieldType : uint8_t { INT = 0,
                                 FLOAT = 1,
                                 STRING = 2 };

class Field {
public:
    FieldType type;
    std::unique_ptr<char[]> data;
    size_t data_length;

    // contructors
    explicit Field(int i);
    explicit Field(float f);
    explicit Field(const std::string &s);

    // rule of five
    ~Field() = default;
    Field(const Field &other);
    Field &operator=(const Field &other);
    Field(Field &&other) noexcept;
    Field &operator=(Field &&other) noexcept;

    // accessors
    FieldType getType() const { return type; }
    int asInt() const;
    float asFloat() const;
    std::string asString() const;

    // serialization
    // Binary layout:
    // [u8 type][u32 length][payload bytes]
    // - INT/FLOAT: length=4, payload = raw bytes
    // - STRING:    length = string.size(), payload = raw bytes
    std::string serialize() const;
    void serialize(std::ofstream &out) const;

    static std::unique_ptr<Field> deserialize(std::istream &in);

    void print(std::ostream &os = std::cout) const;
};

} // namespace srd::record