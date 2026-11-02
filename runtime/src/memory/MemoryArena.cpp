#include "runtime/memory/MemoryArena.h"

#include <algorithm>
#include <stdexcept>

namespace project_los_santos::runtime::memory {

namespace {
std::size_t alignUp(std::size_t value, std::size_t alignment) {
    if (alignment == 0u) {
        return value;
    }
    const std::size_t mask = alignment - 1u;
    return (value + mask) & ~mask;
}
}  // namespace

MemoryArena::MemoryArena(std::size_t capacity)
    : storage_(capacity == 0u ? 0u : capacity), capacity_(capacity) {
    used_ = 0u;
}

void* MemoryArena::allocate(std::size_t size, std::size_t alignment) {
    if (size == 0u) {
        return nullptr;
    }

    const std::size_t alignedOffset = alignUp(used_, alignment);
    const std::size_t required = alignedOffset + size;
    if (required > storage_.size()) {
        return nullptr;
    }

    void* result = storage_.data() + alignedOffset;
    used_ = required;
    return result;
}

void MemoryArena::reset() {
    used_ = 0u;
}

}  // namespace project_los_santos::runtime::memory
