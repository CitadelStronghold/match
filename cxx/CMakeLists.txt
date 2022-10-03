cmake_minimum_required(VERSION 3.23)

project(CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(CXX
    src/Main.cxx
)

target_include_directories(CXX PUBLIC src)