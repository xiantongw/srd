#include "srd/storage/slotted_page.hpp"
#include "gtest/gtest.h"

using srd::record::Field;
using srd::record::Tuple;
using srd::storage::SlottedPage;

static std::unique_ptr<Tuple> makeTuple(int i, float f, const char *s) {
    auto t = std::make_unique<Tuple>();
    t->addField(std::make_unique<Field>(i));
    t->addField(std::make_unique<Field>(f));
    t->addField(std::make_unique<Field>(std::string(s)));
    return t;
}

TEST(SlottedPage, InsertAndGet) {
    SlottedPage p;
    ASSERT_TRUE(p.addTuple(makeTuple(7, 2.5f, "hi")));
    Tuple out;
    EXPECT_TRUE(p.getTuple(0, out));
    ASSERT_EQ(out.fields.size(), 3u);
    EXPECT_EQ(out.fields[0]->asInt(), 7);
    EXPECT_FLOAT_EQ(out.fields[1]->asFloat(), 2.5f);
    EXPECT_EQ(out.fields[2]->asString(), "hi");
}

TEST(SlottedPage, DeleteThenReadFails) {
    SlottedPage p;
    ASSERT_TRUE(p.addTuple(makeTuple(1, 1.0f, "a")));
    p.deleteTuple(0);
    Tuple out;
    EXPECT_FALSE(p.getTuple(0, out));
}

TEST(SlottedPage, MultipleSlotsOffsetsGrow) {
    SlottedPage p;
    ASSERT_TRUE(p.addTuple(makeTuple(1, 1.f, "x")));
    ASSERT_TRUE(p.addTuple(makeTuple(2, 2.f, "yy")));
    Tuple t0, t1;
    EXPECT_TRUE(p.getTuple(0, t0));
    EXPECT_TRUE(p.getTuple(1, t1));
    EXPECT_EQ(t0.fields[0]->asInt(), 1);
    EXPECT_EQ(t1.fields[0]->asInt(), 2);
}