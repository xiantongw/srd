#include <gtest/gtest.h>
#include "include/srd/greeter.h"

TEST(Greeter, SaysHello) {
  EXPECT_EQ(srd::Greet("Tom"), "Hello, Tom!");
}