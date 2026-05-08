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

#include "move_picker.hpp"

namespace kerosene {

auto MovePicker::next_move(bool skip_quiets) -> Move {
    switch (m_stage) {
    case kGenerateMoves: {
        m_moves = generate_legal_moves(m_pos);
        m_scores.clear();
        m_emit_idx = 0;

        m_stage = kScoreMoves;
        [[fallthrough]];
    }

    case kScoreMoves: {
        for (auto move : m_moves) {
            if (m_pos.is_capture(move)) {
                PieceType victim = move.special_type() == Move::kEnPassant
                                   ? PieceType::kPawn
                                   : m_pos.piece_at(move.dst()).piece_type();

                PieceType attacker = m_pos.piece_at(move.src()).piece_type();

                m_scores.emplace_back(static_cast<usize>(victim) * 20
                                      - static_cast<usize>(attacker));
            } else {
                m_scores.emplace_back(0);
            }
        }

        m_stage = kEmitMoves;
        [[fallthrough]];
    }

    case kEmitMoves: {
        if (m_emit_idx >= m_moves.size()) {
            return kNullMove;
        }

        usize best_idx = m_moves.size();
        for (usize idx = m_emit_idx; idx < m_moves.size(); ++idx) {
            if (skip_quiets && !m_pos.is_capture(m_moves[idx])) {
                continue;
            }

            if (best_idx == m_moves.size() || m_scores[idx] > m_scores[best_idx]) {
                best_idx = idx;
            }
        }

        if (best_idx == m_moves.size()) {
            return kNullMove;
        }

        std::swap(m_moves[m_emit_idx], m_moves[best_idx]);
        std::swap(m_scores[m_emit_idx], m_scores[best_idx]);

        return m_moves[m_emit_idx++];
    }
    }

    return kNullMove;
}

}  // namespace kerosene
