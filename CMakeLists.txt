cmake_minimum_required(VERSION 4.1)
project(Kerosene)

set(CMAKE_CXX_STANDARD 23)

add_executable(Kerosene
        src/main.cpp
        src/util/integer_types.hpp
        src/uci.cpp
        src/uci.hpp
        src/types.hpp
        src/position.cpp
        src/position.hpp
        src/move.hpp
        src/move.cpp
        src/util/bits.hpp
        src/util/inplace_vector.hpp
        src/move_generation.cpp
        src/move_generation.hpp
        src/util/simd.hpp
        src/util/template.hpp
        src/bit_board.hpp)
