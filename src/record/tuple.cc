#include "srd/record/tuple.hpp"

#include <cstring>
#include <fstream>

#include "srd/common/io.hpp"

using srd::common::reader;
using srd::common::writer;

namespace srd::record {

void Tuple::addField(std::unique_ptr<Field> f) {
    fields.push_back(std::move(f));
}

size_t Tuple::getSize() const {
    size_t total = 0;
    for (const auto &f : fields)
        if (f) total += f->data_length;
    return total;
}

std::string Tuple::serialize() const {
    std::string blob;
    size_t approx_size = sizeof(uint32_t);
    approx_size += this->getSize();
    blob.reserve(approx_size);

    uint32_t count = static_cast<uint32_t>(fields.size());
    blob.append(reinterpret_cast<const char *>(&count), sizeof(count));

    for (const auto &f : fields) {
        const auto fs = f->serialize();
        blob.append(fs.data(), fs.size());
    }
    return blob;
}

void Tuple::serialize(std::ofstream &out) const {
    uint32_t count = static_cast<uint32_t>(fields.size());
    writer(out, &count, sizeof(count));
    for (const auto &f : fields) {
        const std::string fs = f->serialize();
        writer(out, fs.data(), fs.size());
    }
}

std::unique_ptr<Tuple> Tuple::deserialize(std::istream &in) {
    auto tup = std::make_unique<Tuple>();
    uint32_t count = 0;
    reader(in, &count, sizeof(count));
    tup->fields.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        tup->fields.push_back(Field::deserialize(in));
    }
    return tup;
}

void Tuple::print(std::ostream &os) const {
    bool first = true;
    for (const auto &f : fields) {
        if (!first) os << ' ';
        if (f) f->print(os);
        first = false;
    }
}

}  // namespace srd::record
