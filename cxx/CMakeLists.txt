cmake_minimum_required(VERSION 3.23)

project(match)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS " /MP")
    string(APPEND CMAKE_C_FLAGS " /MP")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /MP")
    string(APPEND CMAKE_C_FLAGS_RELEASE " /MP")
endif ()

add_executable(match
    src/Main.cxx
    src/Validator.cxx
)

target_include_directories(match PUBLIC src)
