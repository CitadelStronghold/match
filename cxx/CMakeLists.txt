cmake_minimum_required(VERSION 3.23)

project(TeekValidator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(TeekValidator
    src/Main.cxx
    src/Validator.cxx
)

target_include_directories(TeekValidator PUBLIC src)