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
#include "score.hpp"
#include "zobrist.hpp"

namespace kerosene {
struct tt_data {
    enum tt_bound : i8 {
        none,
        upper,
        lower,
        exact,
    };

    Move     move;
    i16      score{};
    i16      static_eval{};
    i8       depth{};
    tt_bound bound{};
};

class transposition_table {
    static constexpr usize default_mb = 16;

public:
    transposition_table();
    ~transposition_table();

    [[nodiscard]] auto probe(const Position& position, i32 ply) const -> std::optional<tt_data>;
    auto               write(const Position&   position,
                             i32               ply,
                             Move              move,
                             i32               depth,
                             Score             score,
                             Score             static_eval,
                             tt_data::tt_bound bound) const -> void;

    auto clear() const -> void;

private:
    struct tt_slot {
        ZKey    key{};
        tt_data data;
    };

    static constexpr auto mb_to_size(usize mb) -> usize;

    auto allocate(usize mb) -> void;
    auto destroy() -> void;

    auto ptr(const Position& position) const -> tt_slot*;

    tt_slot* m_data{nullptr};
    usize    m_size{0};
};

}
