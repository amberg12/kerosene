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
namespace {
constexpr i32 kTtMove    = 900'000'000;
constexpr i32 kLoudMove  = 800'000'000;
constexpr i32 kPromotion = 700'000'000;
constexpr i32 kKiller    = 600'000'000;
}

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
            if (move == m_tt_move) {
                m_scores.emplace_back(kTtMove);
            } else if (m_pos.is_capture(move)) {
                PieceType victim = move.special_type() == Move::kEnPassant
                                   ? PieceType::kPawn
                                   : m_pos.piece_at(move.dst()).piece_type();

                PieceType attacker = m_pos.piece_at(move.src()).piece_type();

                m_scores.emplace_back(kLoudMove + static_cast<usize>(victim) * 20
                                      - static_cast<usize>(attacker));
            } else if (move.special_type() == Move::kPromotion) {
                m_scores.emplace_back(kPromotion);
            } else if (move == m_killer) {
                m_scores.emplace_back(kKiller);
            } else {
                const i32 score = [&] {
                    i32 out = m_history.quiet_history.read(m_pos, move);

                    for (const i32 ply_offset : conthist_offsets) {
                        const ss_item& p_ss = m_ss.at(m_ply - ply_offset);

                        if (p_ss.conthist_subtable) {
                            out += p_ss.conthist_subtable->read(m_pos, move);
                        }
                    }

                    return out;
                }();

                m_scores.emplace_back(score);
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
            if (skip_quiets
                && !(m_pos.is_capture(m_moves[idx])
                     || m_moves[idx].special_type() == Move::kPromotion)) {
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
