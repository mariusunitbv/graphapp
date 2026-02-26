cmake_minimum_required(VERSION 3.28)
project(GraphModel)

file(GLOB_RECURSE MODULES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_sources(${PROJECT_NAME} PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    FILES ${MODULES}
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch/pch.h>")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--no-undefined -Wl,--gc-sections)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch")