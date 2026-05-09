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

#include <ranges>
#include "zobrist.hpp"

#include "position.hpp"

#include <random>

namespace kerosene {
namespace {
std::array<std::array<ZKey, 16>, 64> gPieceSquareKey;
ZKey gSideToMove;
std::array<ZKey, 8> gEnPassantFile;
std::array<ZKey, 16> gCastlingRights;
}

auto init_zobrist() -> void {
    namespace rv = std::views;
    namespace rg = std::ranges;

    std::mt19937_64 rng(8008135);  // NOLINT(*-msc51-cpp)

    for (ZKey& key : gPieceSquareKey | rv::join) {
        key = rng();
    }

    gSideToMove = rng();

    for (ZKey& key : gEnPassantFile) {
        key = rng();
    }

    for (ZKey& key : gCastlingRights) {
        key = rng();
    }
}

auto z_key_piece_square(Piece piece, Square square) -> ZKey {
    return gPieceSquareKey[square][static_cast<usize>(piece)];
}

auto z_key_side_to_move() -> ZKey {
    return gSideToMove;
}

auto z_key_en_passant_file(i8 file) -> ZKey {
    return gEnPassantFile[file];
}

auto z_key_castling_rights(const CastlingRights& castling_rights) -> ZKey {
    return gCastlingRights[static_cast<usize>(castling_rights)];
}

}  // namespace kerosene
