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

auto evaluate(const Position& pos) -> Score {
    Score score = 0;

    score += pos.piece_mask_for(Color::kWhite, PieceType::kPawn).popcount()
           - pos.piece_mask_for(Color::kBlack, PieceType::kPawn).popcount();

    score += pos.piece_mask_for(Color::kWhite, PieceType::kKnight).popcount()
           - pos.piece_mask_for(Color::kBlack, PieceType::kKnight).popcount();

    score += pos.piece_mask_for(Color::kWhite, PieceType::kBishop).popcount()
           - pos.piece_mask_for(Color::kBlack, PieceType::kBishop).popcount();

    score += pos.piece_mask_for(Color::kWhite, PieceType::kRook).popcount()
           - pos.piece_mask_for(Color::kBlack, PieceType::kRook).popcount();

    score += pos.piece_mask_for(Color::kWhite, PieceType::kQueen).popcount()
           - pos.piece_mask_for(Color::kBlack, PieceType::kQueen).popcount();

    return pos.side_to_move() == Color::kWhite ? score : -score;
}

}  // namespace kerosene
