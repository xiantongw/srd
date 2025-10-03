#pragma once
#include <vector>

#include "srd/record/field.hpp"

namespace srd::record {

class Tuple {
   public:
    std::vector<std::unique_ptr<Field>> fields;

    Tuple() = default;
    ~Tuple() = default;

    Tuple(const Tuple &) = delete;
    Tuple &operator=(const Tuple &) = delete;

    Tuple(Tuple &&) = default;
    Tuple &operator=(Tuple &&) = default;

    void addField(std::unique_ptr<Field> f);

    size_t getSize() const;

    // [u32 field_count] [Field0] [Field1] ...
    // Each Field is [u8 type][u32 len][len bytes]
    std::string serialize() const;
    void serialize(std::ofstream &out) const;

    static std::unique_ptr<Tuple> deserialize(std::istream &in);

    void print(std::ostream &os = std::cout) const;
};
}  // namespace srd::record