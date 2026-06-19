cmake_minimum_required(VERSION 3.16)
project(OOPL_Mario)

set(CMAKE_CXX_STANDARD 17)

set(GAME_WINDOW_WIDTH 960)
set(GAME_WINDOW_HEIGHT 720)
set(GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hpp.in
    ${GENERATED_INCLUDE_DIR}/config.hpp
    @ONLY
)

cmake_policy(SET CMP0135 NEW)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

FetchContent_Declare(
    ptsd
    URL https://github.com/ntut-open-source-club/practical-tools-for-simple-design/archive/refs/tags/v0.2.zip
    URL_HASH SHA256=4dd744df64f502572348ec0209e5ac05eecf96b68a67f9e639087667ebf1b94d
    PATCH_COMMAND
        ${CMAKE_COMMAND}
        "-DPTSD_SOURCE_DIR=<SOURCE_DIR>"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchPTSD.cmake
)

FetchContent_MakeAvailable(ptsd)
target_include_directories(PTSD BEFORE PRIVATE ${GENERATED_INCLUDE_DIR})

include(files.cmake)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
list(TRANSFORM SRC_FILES PREPEND ${SRC_DIR}/)
list(TRANSFORM INCLUDE_FILES PREPEND ${INCLUDE_DIR}/)

# file(GLOB_RECURSE SRC_FILES src/*)
# file(GLOB_RECURSE HEADER_FILES include/*)

add_executable(${PROJECT_NAME}
    ${SRC_DIR}/main.cpp
    ${SRC_FILES}
    ${INCLUDE_FILES}
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources")
else()
    message(FATAL_ERROR "relative RESOURCE_DIR is WIP, Please use `-DCMAKE_BUILD_TYPE=Debug` build for now.")
    # target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_DIR="${CMAKE_INSTALL_PREFIX}/Resources")
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${GENERATED_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${ptsd_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2main
    PTSD
    nlohmann_json::nlohmann_json
)

add_executable(KoopaBehaviorTests
    tests/KoopaBehaviorTest.cpp
)
target_include_directories(KoopaBehaviorTests PRIVATE
    ${GENERATED_INCLUDE_DIR}
    ${ptsd_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(KoopaBehaviorTests SYSTEM PRIVATE
    ${DEPENDENCY_INCLUDE_DIRS}
)
target_compile_definitions(KoopaBehaviorTests PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources"
)
target_link_libraries(KoopaBehaviorTests
    GTest::gtest_main
    PTSD
)
