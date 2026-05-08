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

#include "evaluation.hpp"

namespace kerosene {
namespace {
template <Color::Underlying kColor, bool kEnableTracing>
auto evaluate_material(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    i32 pawn_count = pos.piece_count(kColor, PieceType::kPawn);
    i32 knight_count = pos.piece_count(kColor, PieceType::kKnight);
    i32 bishop_count = pos.piece_count(kColor, PieceType::kBishop);
    i32 rook_count = pos.piece_count(kColor, PieceType::kRook);
    i32 queen_count = pos.piece_count(kColor, PieceType::kQueen);

    if constexpr (kEnableTracing) {
        using enum tuning::EvalFeature;

        eval_trace->increment_feature<kColor>(kPawnMaterial, pawn_count);
        eval_trace->increment_feature<kColor>(kKnightMaterial, knight_count);
        eval_trace->increment_feature<kColor>(kBishopMaterial, bishop_count);
        eval_trace->increment_feature<kColor>(kRookMaterial, rook_count);
        eval_trace->increment_feature<kColor>(kQueenMaterial, queen_count);
    }

    return ScorePair{};
}
}  // namespace

template <bool kEnableTracing>
auto evaluate(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score {
    ScorePair out{};

    out += evaluate_material<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out -= evaluate_material<Color::kBlack, kEnableTracing>(pos, eval_trace);

    return out.taper(24);
}

template auto evaluate<true>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;
template auto evaluate<false>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;

}  // namespace kerosene
