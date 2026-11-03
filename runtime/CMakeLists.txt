add_library(runtime
    src/gamedata/GameDataCatalog.cpp
    src/gamedata/Xbox360Metadata.cpp
    src/memory/MemoryArena.cpp
)

target_include_directories(runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
