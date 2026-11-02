cmake_minimum_required(VERSION 3.16)

project(Project_Los_Santos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(BUILD_ANDROID "Build the Android native library" OFF)

add_subdirectory(recomp)
add_subdirectory(runtime)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(BUILD_ANDROID)
    add_subdirectory(android)
endif()
