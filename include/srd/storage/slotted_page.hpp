#pragma once
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "srd/record/tuple.hpp"

using srd::record::Tuple;

namespace srd::storage {

inline constexpr size_t PAGE_SIZE = 4096;
inline constexpr size_t MAX_SLOTS = 128;
inline constexpr uint16_t INVALID_VALUE = 0xFFFF;

// Slot metadata stored at the beginning of each page
struct Slot {
    bool empty = true;
    uint16_t offset = INVALID_VALUE;
    uint16_t length = INVALID_VALUE;
};
static_assert(sizeof(Slot) == 6,
              "Slot size changed: update metadata_size() math");

// Slotted page with a fixed slot directory placed at the page start.
// Tuple bytes are the result of Tuple::serialize()
// +---------------------------+  offset 0
// | Slot[0]                   |
// | Slot[1]                   |
// | ...                       |
// | Slot[MAX_SLOTS-1]         |  offset = metadata_size - 1
// +---------------------------+
// | <--   payload region   -->|  offset = metadata_size
// | Tuple bytes ...           |
// | ...                       |
// +---------------------------+  offset = PAGE_SIZE - 1
class SlottedPage {
   public:
    SlottedPage();
    ~SlottedPage() = default;

    // Non-copyable
    SlottedPage(const SlottedPage &) = delete;
    SlottedPage &operator=(const SlottedPage &) = delete;
    // Movable
    SlottedPage(SlottedPage &&) noexcept = default;
    SlottedPage &operator=(SlottedPage &&) noexcept = default;

    // Insert a tuple into the first slot that has enough capacity
    // Returns true if inserted, false if it doesn't fit anywhere or page full.
    bool addTuple(std::unique_ptr<Tuple> tuple);

    // Mark slot as empty; does not reclaim space or shuffle offsets
    bool deleteTuple(size_t index);

    bool getTuple(size_t index, srd::record::Tuple &out) const;

    void print() const;

    size_t metadata_size() const {
        return sizeof(Slot) * MAX_SLOTS;
    }

    // Raw page access for storage I/O (exactly PAGE_SIZE bytes).
    inline char *raw_data() noexcept {
        return page_data_.get();
    }
    inline const char *raw_data() const noexcept {
        return page_data_.get();
    }

    // Useful introspection (not strictly required, but handy in tests)
    std::size_t used_bytes() const;   // sum of live tuple lengths
    std::size_t free_bytes() const {  // available payload space
        const auto used = used_bytes();
        const auto cap = PAGE_SIZE - metadata_size();
        return (cap > used) ? (cap - used) : 0;
    }

   private:
    std::unique_ptr<char[]> page_data_ = std::make_unique<char[]>(PAGE_SIZE);
    size_t used_bytes_(const Slot *slot_array) const;
    size_t tail_end_(const Slot *slot_array) const;
    void compact_();

    static std::shared_ptr<spdlog::logger> logger;
};

}  // namespace srd::storage