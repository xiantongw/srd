#pragma once
#include <istream>
#include <ostream>

namespace srd::common {

// helpers
static void writer(std::ostream &os, const void *p, size_t n) {
    os.write(reinterpret_cast<const char *>(p),
             static_cast<std::streamsize>(n));
    if (!os) throw std::runtime_error("Field::serialize: write failed");
}

static void reader(std::istream &is, void *p, size_t n) {
    is.read(reinterpret_cast<char *>(p), static_cast<std::streamsize>(n));
    if (!is) throw std::runtime_error("Field::serialize: read failed");
}
}  // namespace srd::common