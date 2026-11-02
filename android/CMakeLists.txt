add_library(native_lib SHARED
    src/main/cpp/native-lib.cpp
)

target_include_directories(native_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp
)

find_library(android-log log REQUIRED)
find_library(android-vulkan vulkan REQUIRED)

target_link_libraries(native_lib
    PRIVATE
        runtime
        ${android-log}
        ${android-vulkan}
)
