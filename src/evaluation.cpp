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

std::array<BitBoard, 64> kKingRings = [] {
    constexpr std::array<std::pair<i8, i8>, 8> kOffsets = {{
      {1, 1},    //
      {1, -1},   //
      {-1, -1},  //
      {-1, 1},   //
      {1, 0},    //
      {-1, 0},   //
      {0, 1},    //
      {0, -1}    //
    }};

    std::array<BitBoard, 64> out{};
    for (usize sq_idx = 0; sq_idx < 64; ++sq_idx) {
        Square mid_point{sq_idx};

        BitBoard& bb = out[sq_idx];

        for (auto [file_offset, rank_offset] : kOffsets) {
            bb |= BitBoard{Square{std::clamp(mid_point.file() + file_offset, 0, 7),
                                  std::clamp(mid_point.rank() + rank_offset, 0, 7)}};
        }

        bb &= ~BitBoard{mid_point};
    }

    return out;
}();

template<Color::Underlying>
auto is_passed_pawn(const Position& pos, Square passer) -> bool = delete;

template<>
auto is_passed_pawn<Color::kWhite>(const Position& pos, Square passer) -> bool {
    BitBoard ahead = BitBoard::full() << (8 * (passer.rank() + 1));
    return (pos.pieces(Color::kBlack, PieceType::kPawn)
            & (ahead & BitBoard::file_triplet(passer.file())))
             .pop_count()
        == 0;
}

template<>
auto is_passed_pawn<Color::kBlack>(const Position& pos, Square passer) -> bool {
    BitBoard ahead = BitBoard::full() >> (8 * (passer.mirror().rank() + 1));

    return (pos.pieces(Color::kWhite, PieceType::kPawn)
            & (ahead & BitBoard::file_triplet(passer.file())))
             .pop_count()
        == 0;
}

template<Color::Underlying kColor>
auto is_isolated(const Position& pos, Square src) -> bool {
    auto isolated_field = BitBoard::file_triplet(src.file()) & ~BitBoard::file(src.file());
    return (pos.pieces(kColor, PieceType::kPawn) & isolated_field).pop_count() == 0;
}

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
        Square square     = pos.info_of(id, kColor).first;
        Square abs_square = pos.info_of(id, kColor).first;

        if constexpr (kColor == Color::kBlack) {
            square = square.mirror();
        }

        out += kPawnPsqt[square];
        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kPawnPsqt, square);
        }

        if (is_passed_pawn<kColor>(pos, abs_square)) {
            out += kPasser[square.rank()];

            if constexpr (kEnableTracing) {
                eval_trace->increment_feature<kColor>(tuning::EvalFeature::kPasser, square.rank(),
                                                      1);
            }
        }

        if (is_isolated<kColor>(pos, abs_square)) {
            out += kIsolated;

            if constexpr (kEnableTracing) {
                eval_trace->increment_feature<kColor>(tuning::EvalFeature::kIsolated);
            }
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

        i32 mobility_offset = pos.mobility_of(kColor, id);

        out += kMobilityKnight[mobility_offset];

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kKnightPsqt, square);
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kMobilityKnight,
                                                  mobility_offset, 1);
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

        i32 mobility_offset = pos.mobility_of(kColor, id);

        out += kMobilityBishop[mobility_offset];

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kBishopPsqt, square);
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kMobilityBishop,
                                                  mobility_offset, 1);
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

        i32 mobility_offset = pos.mobility_of(kColor, id);

        out += kMobilityRook[mobility_offset];

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kRookPsqt, square);
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kMobilityRook,
                                                  mobility_offset, 1);
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

        i32 mobility_offset = pos.mobility_of(kColor, id);

        out += kMobilityQueen[mobility_offset];

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kQueenPsqt, square);
            eval_trace->increment_feature<kColor>(tuning::EvalFeature::kMobilityQueen,
                                                  mobility_offset, 1);
        }
    }

    return out;
}

template<Color::Underlying kColor, bool kEnableTracing>
auto evaluate_king(const Position& pos, tuning::EvaluationTrace* eval_trace) -> ScorePair {
    ScorePair out{};

    Square rel_square = pos.king_square(kColor);
    Square abs_square = rel_square;
    if constexpr (kColor == Color::kBlack) {
        rel_square = rel_square.mirror();
    }

    out += kKingPsqt[rel_square];

    if constexpr (kEnableTracing) {
        eval_trace->increment_feature<kColor>(tuning::EvalFeature::kKingPsqt, rel_square);
    }

    for (Square sq : kKingRings[abs_square]) {
        PieceMask attackers = pos.attackers_to(sq)[~Color{kColor}];

        for (PieceId attacker : attackers) {
            auto [square, piece_type] = pos.info_of(attacker, ~Color{kColor});

            out += kKingRing[piece_type];

            if constexpr (kEnableTracing) {
                eval_trace->increment_feature<kColor>(tuning::EvalFeature::kKingRing,
                                                      static_cast<usize>(piece_type), 1);
            }
        }
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

    if (pos.side_to_move() == Color::kWhite) {
        out += kTempo;

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<Color::kWhite>(tuning::EvalFeature::kTempo);
        }
    } else {
        out -= kTempo;

        if constexpr (kEnableTracing) {
            eval_trace->increment_feature<Color::kBlack>(tuning::EvalFeature::kTempo);
        }
    }

    Score score = out.taper(pos.phase());

    return pos.side_to_move() == Color::kWhite ? score : -score;
}

template auto evaluate<true>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;
template auto evaluate<false>(const Position& pos, tuning::EvaluationTrace* eval_trace) -> Score;

}  // namespace kerosene
