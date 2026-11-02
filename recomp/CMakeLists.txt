add_library(recomp
    src/powerpc/PowerPCDecoder.cpp
    src/arm64/Arm64Emitter.cpp
)

target_include_directories(recomp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
