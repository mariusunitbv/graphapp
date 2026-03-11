cmake_minimum_required(VERSION 3.28)
project(GraphModelTest)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch/pch.h>")
target_link_options(${PROJECT_NAME} PRIVATE -Wl,--no-undefined -Wl,--gc-sections)
target_link_libraries(${PROJECT_NAME} PRIVATE
    GraphModel
    GTest::gtest
    GTest::gtest_main
    ZLIB::ZLIB
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/pch"
)