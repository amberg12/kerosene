/*
  Kerosene - A UCI chess engine.
  Copyright (C) 2026 Amber Goulding

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <print>

#include "move.hpp"
#include "position.hpp"
#include "util/inplace_vector.hpp"
#include "util/simd.hpp"

namespace kerosene {
constexpr usize kMaxLegalMoves = kMaxSimdAlign * 6; // 64 * 6 = 384

using MoveList = inplace_vector<Move, kMaxLegalMoves>;

auto generate_legal_moves(const Position& pos) -> MoveList;

template <bool kShouldPrint = true>
[[nodiscard]] auto perft(const Position& position, i32 depth) -> u64 {
    if (depth == 0) {
        return 1;
    }

    MoveList legal_moves = generate_legal_moves(position);

    u64 total_nodes = 0;

    for (Move move : legal_moves) {
        Position child_position{position, move};
        u64 node_count = perft<false>(child_position, depth - 1);
        total_nodes += node_count;

        if (kShouldPrint) {
            std::println("{}: {}", move.to_string(), node_count);
        }
    }

    return total_nodes;
}
}
