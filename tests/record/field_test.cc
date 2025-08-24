#include "srd/record/field.hpp"
#include <gtest/gtest.h>

using srd::record::Field;
using srd::record::FieldType;

static std::unique_ptr<Field> RoundTripBlob(const Field &f) {
    const std::string blob = f.serialize();
    std::istringstream is(std::string(blob.data(), blob.size()), std::ios::binary);
    return Field::deserialize(is);
}

TEST(Field, IntRoundTrip) {
    Field f(42);
    EXPECT_EQ(f.getType(), FieldType::INT);
    EXPECT_EQ(f.asInt(), 42);

    auto g = RoundTripBlob(f);
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->getType(), FieldType::INT);
    EXPECT_EQ(g->asInt(), 42);
}

TEST(Field, FloatRoundTrip) {
    Field f(4.2f);
    EXPECT_EQ(f.getType(), FieldType::FLOAT);
    EXPECT_EQ(f.asFloat(), 4.2f);

    auto g = RoundTripBlob(f);
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->getType(), FieldType::FLOAT);
    EXPECT_EQ(g->asFloat(), 4.2f);
}

TEST(Field, StringRoundTrip) {
    std::string s = "Hello World";
    Field f(s);

    EXPECT_EQ(f.getType(), FieldType::STRING);
    EXPECT_EQ(f.asString(), s);

    const std::string blob = f.serialize();
    ASSERT_GE(blob.size(), size_t{5});
    uint32_t len = 0;
    std::memcpy(&len, blob.data() + sizeof(uint8_t), sizeof(uint32_t));
    EXPECT_EQ(len, 1 + s.size());

    auto g = RoundTripBlob(f);
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->getType(), FieldType::STRING);
    EXPECT_EQ(g->asString(), s);
}

TEST(Field, CopyAndMoveSemantics) {
    Field a{123};
    Field b = a; // copy ctor
    EXPECT_EQ(b.getType(), FieldType::INT);
    EXPECT_EQ(b.asInt(), 123);

    Field c{std::string("zzz")};
    Field d = std::move(c); // move ctor
    EXPECT_EQ(d.getType(), FieldType::STRING);
    EXPECT_EQ(d.asString(), "zzz");

    Field e{3.5f};
    d = e; // copy assign
    EXPECT_EQ(d.getType(), FieldType::FLOAT);
    EXPECT_FLOAT_EQ(d.asFloat(), 3.5f);

    Field f{std::string("yyy")};
    d = std::move(f); // move assign
    EXPECT_EQ(d.getType(), FieldType::STRING);
    EXPECT_EQ(d.asString(), "yyy");
}

TEST(Field, PrintOutputsValue) {
    Field fi(9);
    Field ff(9.6f);
    Field fs(std::string("icu"));

    std::ostringstream os;
    fi.print(os);
    os << ' ';
    ff.print(os);
    os << ' ';
    fs.print(os);

    EXPECT_EQ(os.str(), "9 9.6 icu");
}
