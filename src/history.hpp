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
#include "move.hpp"
#include "position.hpp"
#include "util/integer_types.hpp"


#include <memory>

namespace kerosene {

class History {
public:
    auto update_quiet_history(const Position& pos, i32 depth, Move move) const -> void {
        i16& entry = (*m_quiet_piece_table)[move.dst()][pos.piece_at(move.src()).compressed_idx()];
        entry      = static_cast<i16>(entry + bonus(depth));
        entry      = std::clamp(entry, static_cast<i16>(-kHistoryMax), kHistoryMax);
    }

    auto read_quiet_history(const Position& pos, Move move) {
        return (*m_quiet_piece_table)[move.dst()][pos.piece_at(move.src()).compressed_idx()];
    }

private:
    static constexpr i16 kHistoryMax = 16384;

    static constexpr auto bonus(i32 depth) -> i16 {
        return static_cast<i16>(std::clamp(320 * depth - 400, 0, 2400));
    }

    std::unique_ptr<std::array<std::array<i16, 12>, 64>> m_quiet_piece_table =
      std::make_unique<std::array<std::array<i16, 12>, 64>>();
};

}  // namespace kerosene
