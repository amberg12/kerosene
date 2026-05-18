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
#include "history.hpp"
#include "move_generation.hpp"
#include "position.hpp"
#include "search_stack.hpp"

namespace kerosene {

class MovePicker {
public:
    enum Stage {
        kGenerateMoves,
        kScoreMoves,
        kEmitMoves,
    };

    MovePicker(const Position& pos,
               const Move      tt_move,
               history&        history,
               search_stack&   ss,
               i32             ply,
               const Move      killer) :
        m_pos(pos),
        m_history(history),
        m_tt_move(tt_move),
        m_ss(ss),
        m_ply(ply),
        m_killer(killer) {
    }

    auto next_move(bool skip_quiets = false) -> Move;

private:
    const Position& m_pos;
    history&        m_history;
    Move            m_tt_move;
    search_stack&   m_ss;
    i32             m_ply;
    Move            m_killer;

    Stage m_stage{kGenerateMoves};
    usize m_emit_idx{};

    MoveList                                  m_moves;
    inplace_vector<i32, MoveList::capacity()> m_scores;
};

}  // namespace kerosene
