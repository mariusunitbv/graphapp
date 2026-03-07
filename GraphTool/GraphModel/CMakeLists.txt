cmake_minimum_required(VERSION 3.28)
project(GraphModel)

file(GLOB_RECURSE MODULES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	find_path(OSMIUM_INCLUDE_DIRS "osmium/version.hpp")
	find_path(PARALLEL_HASHMAP_INCLUDE_DIRS "parallel_hashmap/btree.h")
endif()

find_package(simdjson CONFIG REQUIRED)

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

target_link_libraries(${PROJECT_NAME} PRIVATE simdjson::simdjson)
target_include_directories(${PROJECT_NAME} PRIVATE 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/pch"
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	target_include_directories(${PROJECT_NAME} PRIVATE ${OSMIUM_INCLUDE_DIRS})
	target_include_directories(${PROJECT_NAME} PRIVATE ${PARALLEL_HASHMAP_INCLUDE_DIRS})
endif()