add_library(runtime
    src/memory/MemoryArena.cpp
)

target_include_directories(runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
