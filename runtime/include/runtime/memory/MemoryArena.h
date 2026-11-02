#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace project_los_santos::runtime::memory {

class MemoryArena {
public:
    explicit MemoryArena(std::size_t capacity);

    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
    void reset();

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t used() const noexcept { return used_; }
    [[nodiscard]] bool empty() const noexcept { return used_ == 0; }

private:
    std::vector<std::byte> storage_;
    std::size_t used_ = 0;
    std::size_t capacity_ = 0;
};

}  // namespace project_los_santos::runtime::memory
