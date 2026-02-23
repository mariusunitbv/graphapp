cmake_minimum_required(VERSION 4.2.3)
project(GraphTool)

# Debug or Release
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_subdirectory(GraphModel)
add_subdirectory(Application)