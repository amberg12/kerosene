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
#include "types.hpp"
#include "util/integer_types.hpp"

namespace kerosene {
class CastlingRights;

using ZKey = u64;

// Using an alternate method like a singleton to handle the zobrist keys seems to come up with some
// awful assembly.
auto init_zobrist() -> void;

auto z_key_piece_square(Piece piece, Square square) -> ZKey;
auto z_key_side_to_move() -> ZKey;
auto z_key_en_passant_file(i8 file) -> ZKey;
auto z_key_castling_rights(const CastlingRights& castling_rights) -> ZKey;

}
