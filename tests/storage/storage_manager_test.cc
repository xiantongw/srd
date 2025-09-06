#include <gtest/gtest.h>
#include <string>

#include "srd/storage/slotted_page.hpp"
#include "srd/storage/storage_manager.hpp"

using namespace srd::storage;

static std::string temp_path() {
    return std::string("srd_sm_test_") + std::to_string(::getpid()) + ".dat";
}

TEST(StorageManagerTest, CREAT) {
    std::string path = temp_path();
    StorageManager sm(path);
    EXPECT_EQ(sm.num_pages(), 1u);
    std::remove(path.c_str());
}