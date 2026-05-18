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

constexpr i32        history_max = 16384;
constexpr std::array conthist_offsets{1, 2};

constexpr auto bonus(const i32 depth) -> i16 {
    return static_cast<i16>(std::clamp(320 * depth - 400, 0, 2400));
}

constexpr auto malus(const i32 depth) -> i16 {
    return static_cast<i16>(-std::clamp(320 * depth - 400, 0, 1200));
}

template<typename T>
constexpr auto update_with_gravity(T& entry, const i32 bonus) -> void
    requires(std::is_integral_v<T>)
{
    const auto clamped_bonus = static_cast<T>(std::clamp(bonus, -history_max, history_max));
    entry += clamped_bonus - entry * std::abs(clamped_bonus) / history_max;
}

template<typename T>
class piece_to_table {
public:
    using value_type = T;

    [[nodiscard]] auto read(const Position& pos, const Move move) const -> const T& {
        return m_table[move.dst()][pos.piece_at(move.src()).compressed_idx()];
    }

    [[nodiscard]] auto read(const Position& pos, const Move move) -> T& {
        return m_table[move.dst()][pos.piece_at(move.src()).compressed_idx()];
    }

    auto write(const Position& pos, const Move move, const T value) -> void
        requires(std::is_integral_v<T>)
    {
        T& entry = read(pos, move);
        update_with_gravity(entry, value);
    }

private:
    using table = std::array<std::array<T, 12>, 64>;

    table m_table{};
};

using piece_to_history = piece_to_table<i16>;

struct history {
    piece_to_history                 quiet_history;
    piece_to_table<piece_to_history> cont_history;
};

}  // namespace kerosene
