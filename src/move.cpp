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

#include "move.hpp"
#include "position.hpp"

namespace kerosene {
auto Move::parse(const std::string& move, const Position& context) -> Move {
    Square src = *Square::parse(move.substr(0, 2));
    Square dst = *Square::parse(move.substr(2, 2));

    if (move.size() == 5) {
        return Move::create_promotion(src, dst, PieceType::parse(move[4]));
    }

    if (context.piece_at(src).piece_type() == PieceType::kPawn && context.en_passant() == dst) {
        return Move::create_en_passant(src, dst);
    }

    if (context.piece_at(src).piece_type() == PieceType::kKing) {
        int diff = std::abs(src.file() - dst.file());

        if (diff >= 2) {
            return Move::create_castling(src, dst);
        }
    }

    return Move{src, dst};
}
}
