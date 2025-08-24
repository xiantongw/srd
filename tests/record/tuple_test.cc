#include "srd/record/tuple.hpp"
#include <gtest/gtest.h>

using srd::record::Field;
using srd::record::Tuple;

static std::unique_ptr<Tuple> RoundTrip(const Tuple &t) {
    const std::string blob = t.serialize();
    std::istringstream is(std::string(blob.data(), blob.size()), std::ios::binary);
    return Tuple::deserialize(is);
}

TEST(Tuple, BasicAddAndSize) {
    Tuple t;
    t.addField(std::make_unique<Field>(int(42)));
    t.addField(std::make_unique<Field>(float(3.5f)));
    t.addField(std::make_unique<Field>(std::string("hi")));
    // INT=4, FLOAT=4, STRING="hi\0" = 3
    EXPECT_EQ(t.getSize(), 4u + 4u + 3u);
}

TEST(Tuple, PrintFormatting) {
    Tuple t;
    t.addField(std::make_unique<Field>(int(5)));
    t.addField(std::make_unique<Field>(std::string("ok")));
    t.addField(std::make_unique<Field>(float(1.5f)));

    std::ostringstream os;
    t.print(os);
    EXPECT_EQ(os.str(), "5 ok 1.5");
}

TEST(Tuple, RoundTripThreeFields) {
    Tuple t;
    t.addField(std::make_unique<Field>(int(7)));
    t.addField(std::make_unique<Field>(float(2.25f)));
    t.addField(std::make_unique<Field>(std::string("hello")));

    auto u = RoundTrip(t);
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->fields.size(), 3u);
    EXPECT_EQ(u->fields[0]->asInt(), 7);
    EXPECT_FLOAT_EQ(u->fields[1]->asFloat(), 2.25f);
    EXPECT_EQ(u->fields[2]->asString(), "hello");
}

TEST(Tuple, StreamSerializeDeserialize) {
    Tuple t;
    t.addField(std::make_unique<Field>(int(1)));
    t.addField(std::make_unique<Field>(std::string("abc")));
    t.addField(std::make_unique<Field>(float(9.0f)));

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    t.serialize(reinterpret_cast<std::ofstream &>(ss));
    ss.seekg(0);

    auto u = Tuple::deserialize(ss);
    ASSERT_NE(u, nullptr);
    ASSERT_EQ(u->fields.size(), 3u);
    EXPECT_EQ(u->fields[0]->asInt(), 1);
    EXPECT_EQ(u->fields[1]->asString(), "abc");
    EXPECT_FLOAT_EQ(u->fields[2]->asFloat(), 9.0f);
}