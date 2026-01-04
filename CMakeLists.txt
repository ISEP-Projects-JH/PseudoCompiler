cmake_minimum_required(VERSION 3.16)
project(CompilerForNASMCpp LANGUAGES CXX)

include(FetchContent)

# ---- JH-Toolkit configuration (must be BEFORE FetchContent_MakeAvailable) ----
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
# use Release to invalidate tests/examples in JH-Toolkit

FetchContent_Declare(
        JH_Toolkit
        GIT_REPOSITORY https://github.com/JeongHan-Bae/JH-Toolkit.git
        GIT_TAG        1.4.0-dev
)

FetchContent_MakeAvailable(JH_Toolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Force using user's bison from PATH (macOS/Linux cross-platform) ---
find_program(USER_BISON_EXECUTABLE bison)
if (USER_BISON_EXECUTABLE)
    set(BISON_EXECUTABLE ${USER_BISON_EXECUTABLE})
endif()

# --- Find Flex/Bison ---
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)
list(REMOVE_ITEM SOURCES ${SRC_DIR}/parser.cpp)

flex_target(scanner ${SRC_DIR}/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)
bison_target(parser ${SRC_DIR}/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp)
add_flex_bison_dependency(scanner parser)

add_executable(compiler
        ${SOURCES}
        ${FLEX_scanner_OUTPUTS}
        ${BISON_parser_OUTPUTS}
)

target_include_directories(compiler PRIVATE ${INC_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(compiler PRIVATE jh-toolkit)

target_compile_options(compiler PRIVATE -fno-rtti)
