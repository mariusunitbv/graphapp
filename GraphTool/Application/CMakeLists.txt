cmake_minimum_required(VERSION 3.28)
project(Application)

file(GLOB_RECURSE MODULES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

find_package(glad CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(lodepng CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
    DEPENDS ${ASSETS}
)

add_dependencies(${PROJECT_NAME} copy_assets)

target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    FILES ${MODULES}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".html"
    )

    target_link_options(${PROJECT_NAME} PRIVATE
		"--preload-file" "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
	
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
    )
else()
    target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch/pch.h>")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--no-undefined -Wl,--gc-sections)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    GraphModel
    glad::glad
    SDL3::SDL3
    imgui::imgui
    lodepng
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pch"
    "${CMAKE_SOURCE_DIR}/GraphModel/include"
)