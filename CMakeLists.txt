cmake_minimum_required(VERSION 4.1)
project(Kerosene)

set(CMAKE_CXX_STANDARD 23)

set(SRCS
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
        src/bit_board.hpp
        src/evaluation.cpp
        src/evaluation.hpp
        src/score.hpp
        src/search.cpp
        src/search.hpp
        src/time_manager.cpp
        src/time_manager.hpp
        src/evaluation_tuning/evaluation_features.hpp
        src/evaluation_tuning/evaluation_trace.hpp
        src/evaluation_tuning/dataset.hpp
        src/evaluation_tuning/dataset.cpp
        src/evaluation_tuning/sgdm.cpp
        src/evaluation_tuning/sgdm.hpp
        src/evaluation_constants.hpp
        src/move_picker.cpp
        src/move_picker.hpp
        src/zobrist.cpp
        src/zobrist.hpp
        src/repetition_table.cpp
        src/repetition_table.hpp
        src/transposition_table.cpp
        src/transposition_table.hpp
        src/util/math.hpp
)

add_executable(Kerosene
        src/main.cpp
        ${SRCS}
        src/history.hpp
        src/util/defer.hpp
        src/util/macro.hpp
        src/search_stack.hpp
)

add_executable(KeroseneTuning
        src/evaluation_tuning/tuning_main.cpp
        ${SRCS}
)
