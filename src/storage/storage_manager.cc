#include "srd/storage/storage_manager.hpp"

#include <spdlog/spdlog.h>

namespace srd::storage {

StorageManager::StorageManager(std::string path) : path_(std::move(path)) {
    open_or_create_();
    recompute_pages_();
    if (num_pages_ == 0) extend_one();
    spdlog::info("StorageManager opened '{}', pages={}", path_, num_pages_);
}

StorageManager::~StorageManager() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void StorageManager::open_or_create_() {
    std::lock_guard<std::mutex> lock(io_mutex_);

    // try to open the file once
    file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    // if cannot open, create the file
    if (!file_.good()) {
        file_.clear();
        std::ofstream creator(path_, std::ios::out | std::ios::binary);
        creator.close();
        file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    }
    // still cannot open the file
    if (!file_.good()) {
        throw std::runtime_error("StorageManager: cannot open file: " + path_);
    }
}

void StorageManager::recompute_pages_() {
    std::lock_guard<std::mutex> lock(io_mutex_);

    file_.seekg(0, std::ios::end);
    const std::streamoff size = file_.tellg();
    if (size < 0) {
        num_pages_ = 0;
    } else {
        num_pages_ = static_cast<std::size_t>(size) / PAGE_SIZE;
    }
    file_.seekg(0, std::ios::beg);
}

void StorageManager::extend_one() {
    std::lock_guard<std::mutex> lock(io_mutex_);

    file_.seekg(0, std::ios::end);
    std::vector<char> zeros(PAGE_SIZE, 0);
    file_.write(zeros.data(), zeros.size());
    file_.flush();
    file_.seekg(0, std::ios::beg);
    ++num_pages_;
}

void StorageManager::extend_to(std::uint64_t page_id) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (page_id + 1 <= num_pages_) return;
    const std::uint64_t add = page_id + 1 - num_pages_;
    std::vector<char> zeros(PAGE_SIZE, 0);
    file_.seekp(0, std::ios::end);
    for (std::uint64_t i = 0; i < add; ++i) {
        file_.write(zeros.data(), zeros.size());
    }
    file_.flush();
    num_pages_ = static_cast<std::size_t>(add);
}
}  // namespace srd::storage