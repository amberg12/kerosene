cmake_minimum_required(VERSION 4.1)
project(Kerosene)

set(CMAKE_CXX_STANDARD 23)

add_executable(Kerosene
        src/main.cpp
        src/common.hpp
        src/uci.cpp
        src/uci.hpp)
