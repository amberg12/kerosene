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
struct TData {
    enum Bound : i8 {
        None,
        Upper,
        Lower,
        Exact,
    };

    Move  move;
    i16   score{};
    i8    depth{};
    Bound bound{};
};

class TranspositionTable {
    static constexpr usize kDefaultMb = 16;

public:
    TranspositionTable();
    ~TranspositionTable();

    [[nodiscard]] auto probe(const Position& position, i32 ply) const -> std::optional<TData>;
    auto               write(const Position& position,
                             i32             ply,
                             Move            move,
                             i32             depth,
                             Score           score,
                             TData::Bound    bound) const -> void;

    auto clear() const -> void;

private:
    struct TSlot {
        ZKey  key{};
        TData data;
    };

    static constexpr auto mb_to_size(usize mb) -> usize;

    auto allocate(usize mb) -> void;
    auto destroy() -> void;

    auto ptr(const Position& position) const -> TSlot*;

    TSlot* m_data{nullptr};
    usize  m_size{0};
};

}
