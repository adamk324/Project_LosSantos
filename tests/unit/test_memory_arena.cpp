#include "runtime/memory/MemoryArena.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using project_los_santos::runtime::memory::MemoryArena;

int main() {
    MemoryArena arena(256u);
    void* first = arena.allocate(16u, 8u);
    assert(first != nullptr);
    void* second = arena.allocate(24u, 16u);
    assert(second != nullptr);
    assert(reinterpret_cast<std::uintptr_t>(second) % 16u == 0u);

    arena.reset();
    assert(arena.empty());

    std::cout << "Memory arena synthetic validation passed.\n";
    return 0;
}
