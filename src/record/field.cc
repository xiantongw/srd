#include "srd/record/field.hpp"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace srd::record {

// helpers
static void writer(std::ostream &os, const void *p, size_t n) {
    os.write(reinterpret_cast<const char *>(p), static_cast<std::streamsize>(n));
    if (!os)
        throw std::runtime_error("Field::serialize: write failed");
}

static void reader(std::istream &is, void *p, size_t n) {
    is.read(reinterpret_cast<char *>(p), static_cast<std::streamsize>(n));
    if (!is)
        throw std::runtime_error("Field::serialize: read failed");
}

// constructors
Field::Field(int i) : type(FieldType::INT), data_length(sizeof(int)) {
    data = std::make_unique<char[]>(data_length);
    std::memcpy(data.get(), &i, data_length);
}

Field::Field(float f) : type(FieldType::FLOAT), data_length(sizeof(float)) {
    data = std::make_unique<char[]>(data_length);
    std::memcpy(data.get(), &f, data_length);
}

Field::Field(const std::string &s) : type(FieldType::STRING) {
    data_length = s.size() + 1; // including the null-terminator
    data = std::make_unique<char[]>(data_length);
    std::memcpy(data.get(), s.c_str(), data_length);
}

// copy constructor
Field::Field(const Field &other) : type(other.type), data_length(other.data_length) {
    if (data_length > 0) {
        data = std::make_unique<char[]>(data_length);
        std::memcpy(data.get(), other.data.get(), data_length);
    }
}

// copy assign constructor
Field &Field::operator=(const Field &other) {
    if (this == &other)
        return *this;
    type = other.type;
    data_length = other.data_length;
    if (data_length > 0) {
        auto tmp = std::make_unique<char[]>(data_length);
        std::memcpy(tmp.get(), other.data.get(), data_length);
        data = std::move(tmp);
    } else {
        data.reset();
    }
    return *this;
}

// move constructor
Field::Field(Field &&other) noexcept {
    type = other.type;
    data_length = other.data_length;
    data = std::move(other.data);
    other.data_length = 0;
}

// move assign constructor
Field &Field::operator=(Field &&other) noexcept {
    if (this == &other)
        return *this;
    type = other.type;
    data = std::move(other.data);
    data_length = other.data_length;
    other.data_length = 0;
    return *this;
}

// accesors
int Field::asInt() const {
    if (type != FieldType::INT || data_length != sizeof(int))
        throw std::runtime_error("Field: not int");
    int v = 0;
    std::memcpy(&v, data.get(), sizeof(int));
    return v;
}

float Field::asFloat() const {
    if (type != FieldType::FLOAT || data_length != sizeof(float)) {
        throw std::runtime_error("Field: not float");
    }
    float v = 0.0;
    std::memcpy(&v, data.get(), sizeof(float));
    return v;
}

std::string Field::asString() const {
    if (type != FieldType::STRING) {
        throw std::runtime_error("Field: not string");
    }
    return std::string(data.get());
}

// ---- serialization ----
// Format: [u8 type][u32 length][payload bytes]
std::string Field::serialize() const {
    int size_type = sizeof(uint8_t);
    int size_length = sizeof(uint32_t);

    std::string blob;
    blob.resize(size_type + size_length + data_length);
    uint8_t *p = reinterpret_cast<uint8_t *>(&blob[0]);

    // type
    *p = static_cast<uint8_t>(type);
    p += size_type;
    // length
    uint32_t len = static_cast<uint32_t>(data_length);
    std::memcpy(p, &len, size_length);
    p += size_length;
    // payload
    if (data_length > 0) {
        std::memcpy(p, data.get(), data_length);
    }
    return blob;
}

void Field::serialize(std::ofstream &out) const {
    const std::string blob = serialize();
    writer(out, blob.data(), blob.size());
}

// deserialization
std::unique_ptr<Field> Field::deserialize(std::istream &in) {
    int size_type = sizeof(uint8_t);
    int size_length = sizeof(uint32_t);

    uint8_t t_raw;
    reader(in, &t_raw, size_type);
    FieldType t = static_cast<FieldType>(t_raw);

    uint32_t len = 0;
    reader(in, &len, size_length);

    if (t == FieldType::INT) {
        if (len != sizeof(int))
            throw std::runtime_error("Field::deserialize int: bad length");
        int v;
        reader(in, &v, sizeof(int));
        return std::make_unique<Field>(v);
    } else if (t == FieldType::FLOAT) {
        if (len != sizeof(float))
            throw std::runtime_error("Field::deserialize float: bad length");
        float v;
        reader(in, &v, sizeof(float));
        return std::make_unique<Field>(v);
    } else if (t == FieldType::STRING) {
        std::vector<char> buffer;
        if (len > 0) {
            buffer.resize(len);
            reader(in, buffer.data(), len);
        }
        return std::make_unique<Field>(std::string(buffer.data()));
    }
    throw std::runtime_error("Field::deserialize: unknown type");
}

// print function
void Field::print(std::ostream &os) const {
    switch (type) {
    case FieldType::INT:
        os << asInt();
        break;
    case FieldType::FLOAT:
        os << asFloat();
        break;
    case FieldType::STRING:
        os << asString();
        break;
    default:
        os << "<unknown>";
        break;
    }
}
} // namespace srd::record
