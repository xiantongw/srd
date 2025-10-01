#include "srd/storage/slotted_page.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>

namespace srd::storage {

std::shared_ptr<spdlog::logger> SlottedPage::logger = [] {
    auto lg = spdlog::stdout_color_mt("slotted_page");
    lg->set_level(spdlog::level::debug);
    return lg;
}();

SlottedPage::SlottedPage() {
    Slot *slots = reinterpret_cast<Slot *>(page_data_.get());
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        slots[i].empty = true;
        slots[i].offset = INVALID_VALUE;
        slots[i].length = INVALID_VALUE;
    }
}

bool SlottedPage::addTuple(std::unique_ptr<Tuple> tuple) {
    const std::string serialized = tuple->serialize();
    const size_t tuple_size = serialized.size();
    if (tuple_size > std::numeric_limits<uint16_t>::max()) return false;

    Slot *slots = reinterpret_cast<Slot *>(page_data_.get());
    const size_t meta_size = metadata_size();

    size_t slot_id = 0;
    for (; slot_id < MAX_SLOTS; ++slot_id) {
        if (slots[slot_id].empty) break;
    }

    if (slot_id == MAX_SLOTS) {
        logger->info("reached maximum slot number.");
        return false;
    }

    auto add_tuple_helper = [&](bool do_compact) -> bool {
        if (do_compact) {
            // check again if the record can fit in the total free space
            const size_t used = used_bytes_(slots);
            const size_t capacity = PAGE_SIZE - meta_size;
            const size_t free = (capacity > used) ? (capacity - used) : 0;
            if (tuple_size > free) {
                logger->info("no more free space to compact.");
                return false;
            }
            compact_();  // if there is enough free space, call compact()
        }

        // now the empty space are all at the tail of the page
        size_t offset = tail_end_(slots);
        if (offset + tuple_size > PAGE_SIZE) {
            logger->info(
                "new tuple added will exceed the PAGE_SIZE limit. \
                          tail: {}, tuple size: {}",
                offset, tuple_size);
            return false;
        }
        if (offset > std::numeric_limits<uint16_t>::max()) {
            logger->info("offset exceeded the numeric limit.");
            return false;
        }

        slots[slot_id].empty = false;
        slots[slot_id].offset = static_cast<uint16_t>(offset);
        slots[slot_id].length = static_cast<uint16_t>(tuple_size);

        std::memcpy(page_data_.get() + offset, serialized.data(), tuple_size);
        return true;
    };

    if (add_tuple_helper(false)) return true;

    return add_tuple_helper(true);
}

void SlottedPage::compact_() {
    logger->info("compact is called.");
    Slot *slots = reinterpret_cast<Slot *>(page_data_.get());
    constexpr int16_t INVALID = -1;

    // array to store the offset and tuple_id
    std::vector<int16_t> offset_to_slot(PAGE_SIZE, INVALID);
    size_t used = 0;
    for (uint16_t i = 0; i < MAX_SLOTS; ++i) {
        const auto &s = slots[i];
        if (!s.empty && s.offset != INVALID_VALUE &&
            s.length != INVALID_VALUE) {
            offset_to_slot[s.offset] = static_cast<int16_t>(i);
            ++used;
        }
    }

    if (used == 0) return;

    size_t cursor = metadata_size();
    for (size_t offset = metadata_size(); offset < PAGE_SIZE;) {
        int16_t slot_id = offset_to_slot[offset];
        if (slot_id == INVALID) {
            ++offset;
            continue;
        }
        Slot &s = slots[slot_id];
        const size_t seg_len = s.length;
        if (offset != cursor) {
            std::memmove(page_data_.get() + cursor, page_data_.get() + offset,
                         seg_len);
            s.offset = static_cast<uint16_t>(cursor);
        }
        cursor += seg_len;
        offset += seg_len;
    }
    if (cursor < PAGE_SIZE) {
        std::memset(page_data_.get() + cursor, 0, PAGE_SIZE - cursor);
        logger->info("compact finished, {} free bytes at tail.",
                     PAGE_SIZE - cursor);
    }
}

bool SlottedPage::deleteTuple(size_t index) {
    if (index >= MAX_SLOTS) return false;

    Slot *slots = reinterpret_cast<Slot *>(page_data_.get());

    if (!slots[index].empty) {
        slots[index].empty = true;
    }

    return true;
}

bool SlottedPage::getTuple(size_t index, srd::record::Tuple &out) const {
    if (index >= MAX_SLOTS) return false;
    const Slot *slots = reinterpret_cast<const Slot *>(page_data_.get());
    const Slot &s = slots[index];
    if (s.empty || s.offset == INVALID_VALUE || s.length == INVALID_VALUE)
        return false;

    // Construct bounded string from the page bytes (no over-read)
    const char *tuple_data = page_data_.get() + s.offset;
    std::string rec(tuple_data, s.length);
    std::istringstream iss(rec);
    auto tup = srd::record::Tuple::deserialize(iss);
    if (!tup) return false;
    out = std::move(*tup);
    return true;
}

void SlottedPage::print() const {
    const Slot *slots = reinterpret_cast<const Slot *>(page_data_.get());
    std::cout << std::endl;
    std::cout << "current tail: " << tail_end_(slots) << std::endl;
    std::cout << "used bytes: " << used_bytes_(slots) << std::endl;
    std::cout << "free bytes: "
              << PAGE_SIZE - metadata_size() - used_bytes_(slots) << std::endl;
    std::cout << std::endl;
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        const Slot &s = slots[i];
        if (!s.empty) {
            assert(s.offset != INVALID_VALUE);
            const char *tuple_data = page_data_.get() + s.offset;
            std::string rec(tuple_data, s.length);  // bounded
            std::istringstream iss(rec);
            auto loaded = srd::record::Tuple::deserialize(iss);
            std::cout << "Slot " << i << " : [" << s.offset << "] : ["
                      << s.length << "] ::";
            if (loaded) loaded->print();
            std::cout << std::endl;
        }
    }
}

size_t SlottedPage::used_bytes_(const Slot *slots) const {
    size_t used = 0;
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        const Slot &s = slots[i];
        if (!s.empty && s.length != INVALID_VALUE &&
            s.offset != INVALID_VALUE) {
            used += s.length;
        }
    }
    return used;
}

size_t SlottedPage::tail_end_(const Slot *slots) const {
    size_t tail = metadata_size();
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        const Slot &s = slots[i];
        if (!s.empty && s.length != INVALID_VALUE &&
            s.offset != INVALID_VALUE) {
            size_t end = static_cast<size_t>(s.offset) + s.length;
            if (end > tail) tail = end;
        }
    }
    return tail;
}

}  // namespace srd::storage