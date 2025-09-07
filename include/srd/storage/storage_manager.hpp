#pragma once

#include "srd/storage/slotted_page.hpp"
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace srd::storage {

class StorageManager {
public:
    explicit StorageManager(std::string path = "srd.dat");

    StorageManager(const StorageManager &) = delete;
    StorageManager &operator=(const StorageManager &) = delete;

    // getter function for the num_pages_
    std::size_t num_pages() const noexcept { return num_pages_; }

    // Ensure the file has at least (page_id + 1) pages (0-based page IDs).
    void extend_to(std::uint64_t page_id);

    // Append a single empty page to the file.
    void extend_one();

    // Read page 'page_id' from disk into a new SlottedPage.
    // Throws std::out_of_range if page_id >= num_pages().
    std::unique_ptr<SlottedPage> load(std::uint64_t page_id);

    // Write the given page to disk at 'page_id'. Page must be PAGE_SIZE bytes.
    void flush(std::uint64_t page_id, const SlottedPage &page);

    // Path of the backing file (useful in tests / logging)
    const std::string &path() const noexcept { return path_; }

    ~StorageManager();

private:
    void open_or_create_();
    void recompute_pages_();

private:
    std::string path_;
    std::fstream file_;
    std::size_t num_pages_ = 0;
    mutable std::mutex io_mutex_;
};

} // namespace srd::storage