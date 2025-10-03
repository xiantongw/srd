#include "srd/storage/storage_manager.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <stdexcept>
#include <vector>

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

    file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.good()) {
        file_.clear();
        std::ofstream creator(path_, std::ios::out | std::ios::binary);
        creator.close();
        file_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    }
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
    file_.seekp(0, std::ios::end);
    std::vector<char> zeros(PAGE_SIZE, 0);
    file_.write(zeros.data(), zeros.size());
    file_.flush();
    ++num_pages_;
}

void StorageManager::extend_to(std::uint64_t page_id) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (page_id + 1 <= num_pages_) return;

    const std::uint64_t add = (page_id + 1) - num_pages_;
    std::vector<char> zeros(PAGE_SIZE, 0);
    file_.seekp(0, std::ios::end);
    for (std::uint64_t i = 0; i < add; ++i) {
        file_.write(zeros.data(), zeros.size());
    }
    file_.flush();
    num_pages_ +=
        static_cast<std::size_t>(add);  // ← fix: increase, don’t overwrite
}

std::unique_ptr<SlottedPage> StorageManager::load(std::uint64_t page_id) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (page_id >= num_pages_) {
        throw std::out_of_range("StorageManager::load page_id out of range");
    }

    auto page = std::make_unique<SlottedPage>();
    file_.seekg(static_cast<std::streamoff>(page_id) * PAGE_SIZE,
                std::ios::beg);
    file_.read(page->raw_data(), PAGE_SIZE);
    if (file_.gcount() != static_cast<std::streamsize>(PAGE_SIZE)) {
        throw std::runtime_error("StorageManager::load short read");
    }
    return page;
}

void StorageManager::flush(std::uint64_t page_id, const SlottedPage& page) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (page_id >= num_pages_) {
        throw std::out_of_range("StorageManager::flush page_id out of range");
    }

    file_.seekp(static_cast<std::streamoff>(page_id) * PAGE_SIZE,
                std::ios::beg);
    file_.write(page.raw_data(), PAGE_SIZE);
    file_.flush();
}

}  // namespace srd::storage