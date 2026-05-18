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
#include "history.hpp"
#include "move.hpp"

namespace kerosene {

struct ss_item {
    Move              killer_move = kNullMove;
    piece_to_history* conthist_subtable{};
};

class search_stack {
public:
    search_stack() = default;

    [[nodiscard]] auto at(i32 ply) const -> const ss_item& {
        return m_ss[idx(ply)];
    }

    [[nodiscard]] auto at(i32 ply) -> ss_item& {
        return m_ss[idx(ply)];
    }

private:
    static constexpr auto idx(i32 ply) -> i32 {
        return ply + 10;
    }

    std::array<ss_item, 300> m_ss{};
};

}
