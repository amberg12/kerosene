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

#include "transposition_table.hpp"
#include "position.hpp"
#include "score.hpp"

#include <cstring>

namespace kerosene {
namespace {
auto score_to_tt(Score score, i32 ply) -> i16 {
    if (!is_mate(score)) {
        return static_cast<i16>(score);
    }

    return score > 0 ? score + ply : score - ply;
}

auto tt_to_score(i16 score, i32 ply) -> i16 {
    if (!is_mate(score)) {
        return static_cast<i16>(score);
    }

    return score > 0 ? score - ply : score + ply;
}
}

constexpr auto TranspositionTable::mb_to_size(usize mb) -> usize {
    return mb * 1024 * 1024 / sizeof(*m_data);
}

TranspositionTable::TranspositionTable() {
    allocate(kDefaultMb);
}

TranspositionTable::~TranspositionTable() {
    destroy();
}

auto TranspositionTable::probe(const Position& position, i32 ply) const -> std::optional<TData> {
    TSlot slot = *ptr(position);

    TData data = slot.data;
    data.score = tt_to_score(data.score, ply);

    return data.bound != TData::None && slot.key == position.hash() ? std::make_optional(data)
                                                                    : std::nullopt;
}

auto TranspositionTable::write(
  const Position& position, i32 ply, Move move, i32 depth, Score score, TData::Bound bound) const
  -> void {
    TSlot* slot = ptr(position);

    // If this position had a previous alpha raise, we can trust its move even if we did not raise
    // alpha again.
    if (!move && slot->key == position.hash()) {
        move = slot->data.move;
    }

    slot->key  = position.hash();
    slot->data = {
      .move  = move,
      .score = score_to_tt(score, ply),
      .depth = static_cast<i8>(depth),
      .bound = bound,
    };
}

auto TranspositionTable::clear() const -> void {
    std::memset(m_data, 0, m_size * sizeof(*m_data));
}

auto TranspositionTable::allocate(usize mb) -> void {
    m_data = new TSlot[mb_to_size(mb)];
    m_size = mb_to_size(mb);
    clear();
}

auto TranspositionTable::destroy() -> void {
    delete[] m_data;
    m_data = nullptr;
}

auto TranspositionTable::ptr(const Position& position) const -> TSlot* {
    usize idx = position.hash() % m_size;
    return m_data + idx;
}
}  // namespace kerosene
