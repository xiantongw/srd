#include "include/srd/greeter.hpp"
#include <gtest/gtest.h>

TEST(Greeter, SaysHello) {
    EXPECT_EQ(srd::Greet("Tom"), "Hello, Tom!");
}