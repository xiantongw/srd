#include "srd/storage/slotted_page.hpp"
#include <cassert>
#include <sstream>

namespace srd::storage {

SlottedPage::SlottedPage() {
    Slot *slot_array = reinterpret_cast<Slot *>(page_data_.get());
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        slot_array[i].empty = true;
        slot_array[i].offset = INVALID_VALUE;
        slot_array[i].length = INVALID_VALUE;
    }
}

bool SlottedPage::addTuple(std::unique_ptr<Tuple> tuple) {
    const std::string serialized = tuple->serialize();
    const size_t tuple_size = serialized.size();

    Slot *slot_array = reinterpret_cast<Slot *>(page_data_.get());

    size_t slot_id = 0;
    for (; slot_id < MAX_SLOTS; ++slot_id) {
        if (slot_array[slot_id].empty)
            break;
    }

    if (slot_id == MAX_SLOTS)
        return false;

    // Calculate offset for new tuple
    size_t offset = metadata_size();
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        if (!slot_array[i].empty &&
            slot_array[i].offset != INVALID_VALUE &&
            slot_array[i].length != INVALID_VALUE) {
            size_t end = static_cast<size_t>(slot_array[i].offset) +
                         static_cast<size_t>(slot_array[i].length);
            if (end > offset)
                offset = end;
        }
    }

    if (offset + tuple_size > PAGE_SIZE)
        return false;

    assert(offset <= std::numeric_limits<uint16_t>::max());
    assert(tuple_size <= std::numeric_limits<uint16_t>::max());

    slot_array[slot_id].empty = false;
    slot_array[slot_id].offset = static_cast<uint16_t>(offset);
    slot_array[slot_id].length = static_cast<uint16_t>(tuple_size);

    std::memcpy(page_data_.get() + offset, serialized.data(), tuple_size);
    return true;
}

void SlottedPage::deleteTuple(size_t index) {
    if (index >= MAX_SLOTS)
        return;

    Slot *slot_array = reinterpret_cast<Slot *>(page_data_.get());

    if (!slot_array[index].empty) {
        slot_array[index].empty = true;
    }
}

bool SlottedPage::getTuple(size_t index, srd::record::Tuple &out) const {
    if (index >= MAX_SLOTS)
        return false;
    const Slot *slot_array = reinterpret_cast<const Slot *>(page_data_.get());
    const Slot &s = slot_array[index];
    if (s.empty || s.offset == INVALID_VALUE || s.length == INVALID_VALUE)
        return false;

    // Construct bounded string from the page bytes (no over-read)
    const char *tuple_data = page_data_.get() + s.offset;
    std::string rec(tuple_data, s.length);
    std::istringstream iss(rec);
    auto tup = srd::record::Tuple::deserialize(iss);
    if (!tup)
        return false;
    out = std::move(*tup);
    return true;
}

void SlottedPage::print() const {
    const Slot *slot_array = reinterpret_cast<const Slot *>(page_data_.get());
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        const Slot &s = slot_array[i];
        if (!s.empty) {
            assert(s.offset != INVALID_VALUE);
            const char *tuple_data = page_data_.get() + s.offset;
            std::string rec(tuple_data, s.length); // bounded
            std::istringstream iss(rec);
            auto loaded = srd::record::Tuple::deserialize(iss);
            std::cout << "Slot " << i << " : [" << s.offset << "] :: ";
            if (loaded)
                loaded->print();
        }
    }
    std::cout << "\n";
}

} // namespace srd::storage