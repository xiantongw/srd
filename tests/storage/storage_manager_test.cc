#include "srd/storage/storage_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <random>
#include <string>

#include "srd/storage/slotted_page.hpp"

using srd::storage::PAGE_SIZE;
using srd::storage::SlottedPage;
using srd::storage::StorageManager;

// Simple unique-ish temp file name inside Bazel sandbox
static std::string tmp_db_path(const char* tag) {
    auto now =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<unsigned long long>(now));
    return std::string("srd_sm_") + tag + "_" + std::to_string(rng()) + ".dat";
}

TEST(StorageManagerTest, CreatesFileAndHasAtLeastOnePage) {
    auto path = tmp_db_path("create");
    StorageManager sm(path);
    EXPECT_GE(sm.num_pages(), 1u);
    // No exception on simple load of page 0
    auto p0 = sm.load(0);
    EXPECT_NE(p0, nullptr);
}

TEST(StorageManagerTest, ExtendToAndExtendOneIncreasePageCount) {
    auto path = tmp_db_path("extend");
    StorageManager sm(path);

    const auto initial = sm.num_pages();
    sm.extend_to(5);  // ensure pages [0..5]
    EXPECT_EQ(sm.num_pages(), std::max<std::size_t>(initial, 6u));

    const auto before_one = sm.num_pages();
    sm.extend_one();
    EXPECT_EQ(sm.num_pages(), before_one + 1);
}

TEST(StorageManagerTest, FlushThenLoadRoundtripPattern) {
    auto path = tmp_db_path("roundtrip");
    StorageManager sm(path);
    sm.extend_to(1);  // make sure we have at least 2 pages

    // Prepare a page with a known pattern
    auto page = std::make_unique<SlottedPage>();
    std::memset(page->raw_data(), 0xAB, PAGE_SIZE);

    sm.flush(0, *page);

    auto reread = sm.load(0);
    ASSERT_NE(reread, nullptr);
    // Spot-check a few bytes
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(static_cast<unsigned char>(reread->raw_data()[i]), 0xAB)
            << "Byte " << i << " mismatched";
    }
}

TEST(StorageManagerTest, PersistsAcrossReopen) {
    auto path = tmp_db_path("persist");

    {
        StorageManager sm(path);
        sm.extend_to(2);  // ensure page 2 exists

        // Write unique data to page 2
        auto p = std::make_unique<SlottedPage>();
        for (size_t i = 0; i < PAGE_SIZE; ++i) {
            p->raw_data()[i] =
                static_cast<char>(i % 251);  // nontrivial pattern
        }
        sm.flush(2, *p);
    }

    // Reopen a new manager and verify content is still there
    {
        StorageManager sm(path);
        auto q = sm.load(2);
        ASSERT_NE(q, nullptr);
        for (size_t i = 0; i < 256; ++i) {
            EXPECT_EQ(static_cast<unsigned char>(q->raw_data()[i]),
                      static_cast<unsigned char>(i % 251))
                << "Byte " << i << " mismatched after reopen";
        }
    }
}

TEST(StorageManagerTest, LoadAndFlushOutOfRangeThrow) {
    auto path = tmp_db_path("oob");
    StorageManager sm(path);
    const auto n = sm.num_pages();
    // out-of-range page id is n (since valid range is [0..n-1])
    EXPECT_THROW(sm.load(n), std::out_of_range);

    auto p = std::make_unique<SlottedPage>();
    EXPECT_THROW(sm.flush(n, *p), std::out_of_range);
}