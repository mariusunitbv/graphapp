cmake_minimum_required(VERSION 3.28)
project(GraphTool)

# Debug or Release
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(GraphCommon)
add_subdirectory(GraphModel)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_subdirectory(GraphModelTest)
endif()

add_subdirectory(Application)