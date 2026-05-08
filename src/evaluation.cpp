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
#include "evaluation_constants.hpp"

namespace kerosene {
using namespace kerosene::evaluation_constants;

namespace {
template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_material(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    i32 pawn_count   = pos.piece_count(kColor, PieceType::kPawn);
    i32 knight_count = pos.piece_count(kColor, PieceType::kKnight);
    i32 bishop_count = pos.piece_count(kColor, PieceType::kBishop);
    i32 rook_count   = pos.piece_count(kColor, PieceType::kRook);
    i32 queen_count  = pos.piece_count(kColor, PieceType::kQueen);

    if constexpr (kEnableTracing) {
        using enum tuning::EvalFeature;

        eval_trace->increment_feature<kColor>(kPawnMaterial, pawn_count);
        eval_trace->increment_feature<kColor>(kKnightMaterial, knight_count);
        eval_trace->increment_feature<kColor>(kBishopMaterial, bishop_count);
        eval_trace->increment_feature<kColor>(kRookMaterial, rook_count);
        eval_trace->increment_feature<kColor>(kQueenMaterial, queen_count);
    }

    return kPawnMaterial * pawn_count + kKnightMaterial * knight_count
         + kBishopMaterial * bishop_count + kRookMaterial * rook_count
         + kQueenMaterial * queen_count;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_pawns(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    for (PieceId id : pos.piece_mask_for(kColor, PieceType::kPawn)) {
        Square square = pos.info_of(id, kColor).first;
        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kPawnPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kPawnPsqt, square);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_knights(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    for (PieceId id : pos.piece_mask_for(kColor, PieceType::kKnight)) {
        Square square = pos.info_of(id, kColor).first;
        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kKnightPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kKnightPsqt, square);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_bishops(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    for (PieceId id : pos.piece_mask_for(kColor, PieceType::kBishop)) {
        Square square = pos.info_of(id, kColor).first;
        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kBishopPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kBishopPsqt, square);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_rooks(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    for (PieceId id : pos.piece_mask_for(kColor, PieceType::kRook)) {
        Square square = pos.info_of(id, kColor).first;
        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kRookPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kRookPsqt, square);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_queens(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    for (PieceId id : pos.piece_mask_for(kColor, PieceType::kQueen)) {
        Square square = pos.info_of(id, kColor).first;
        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kQueenPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kQueenPsqt, square);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_king(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    Square square = pos.king_square(kColor);
    if constexpr (kColor == Color::kBlack) {
        square = square.mirror();
    }

    out += kKingPsqt[square];

    if constexpr (kEnableTracing) {
        eval_trace->increment_feature<kColor>(tuning::EvalFeature::kKingPsqt, square);
    }

    return out;
}

}  // namespace

template<bool kEnableTracing>
auto evaluate(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score {
    ScorePair out{};

    out += evaluate_material<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out -= evaluate_material<Color::kBlack, kEnableTracing>(pos, eval_trace);

    out += evaluate_pawns<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out += evaluate_knights<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out += evaluate_bishops<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out += evaluate_rooks<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out += evaluate_queens<Color::kWhite, kEnableTracing>(pos, eval_trace);
    out += evaluate_king<Color::kWhite, kEnableTracing>(pos, eval_trace);

    out -= evaluate_pawns<Color::kBlack, kEnableTracing>(pos, eval_trace);
    out -= evaluate_knights<Color::kBlack, kEnableTracing>(pos, eval_trace);
    out -= evaluate_bishops<Color::kBlack, kEnableTracing>(pos, eval_trace);
    out -= evaluate_rooks<Color::kBlack, kEnableTracing>(pos, eval_trace);
    out -= evaluate_queens<Color::kBlack, kEnableTracing>(pos, eval_trace);
    out -= evaluate_king<Color::kBlack, kEnableTracing>(pos, eval_trace);

    Score score = out.taper(pos.phase());

    return pos.side_to_move() == Color::kWhite ? score : -score;
}

template auto evaluate<true>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;
template auto evaluate<false>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;

}  // namespace kerosene
