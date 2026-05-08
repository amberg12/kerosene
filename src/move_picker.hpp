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
#include "move_generation.hpp"
#include "position.hpp"

namespace kerosene {

class MovePicker {
public:
    enum Stage {
        kGenerateMoves,
        kScoreMoves,
        kEmitMoves,
    };

    MovePicker(const Position& pos) :
        m_pos(pos) {
    }

    auto next_move() -> Move;

private:
    const Position& m_pos;

    Stage m_stage{kGenerateMoves};
    usize m_emit_idx{};

    MoveList                                  m_moves;
    inplace_vector<i32, MoveList::capacity()> m_scores;
};

}  // namespace kerosene
