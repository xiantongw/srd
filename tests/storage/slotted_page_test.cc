#include "srd/storage/slotted_page.hpp"
#include "gtest/gtest.h"
#include <string>

using srd::record::Field;
using srd::record::Tuple;
using srd::storage::SlottedPage;

static std::unique_ptr<Tuple> makeTuple(int i, float f, std::string s) {
    auto t = std::make_unique<Tuple>();
    t->addField(std::make_unique<Field>(i));
    t->addField(std::make_unique<Field>(f));
    t->addField(std::make_unique<Field>(s));
    return t;
}

TEST(SlottedPage, InsertAndGet) {
    SlottedPage p;
    ASSERT_TRUE(p.addTuple(makeTuple(7, 8.8f, "to_be_deleted")));
    Tuple out;
    std::string base_str = "hello ";
    for (int i = 0; i < 91; i++) {
        ASSERT_TRUE(p.addTuple(makeTuple(i, 8.8f, base_str + std::to_string(i + 1))));
    }
    ASSERT_TRUE(p.deleteTuple(0));
    ASSERT_TRUE(p.addTuple(makeTuple(0, 8.8f, base_str + std::to_string(1))));
    ASSERT_FALSE(p.addTuple(makeTuple(1, 8.8f, base_str + std::to_string(2))));
}

TEST(SlottedPage, DeleteThenReadFails) {
    SlottedPage p;
    ASSERT_TRUE(p.addTuple(makeTuple(1, 1.0f, "a")));
    p.deleteTuple(0);
    Tuple out;
    EXPECT_FALSE(p.getTuple(0, out));
}