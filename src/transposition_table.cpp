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

constexpr auto transposition_table::mb_to_size(usize mb) -> usize {
    return mb * 1024 * 1024 / sizeof(*m_data);
}

transposition_table::transposition_table() {
    allocate(default_mb);
}

transposition_table::~transposition_table() {
    destroy();
}

auto transposition_table::probe(const Position& position, i32 ply) const -> std::optional<tt_data> {
    tt_slot slot = *ptr(position);

    tt_data data = slot.data;
    data.score   = tt_to_score(data.score, ply);

    return data.bound != tt_data::none && slot.key == position.hash() ? std::make_optional(data)
                                                                      : std::nullopt;
}

auto transposition_table::write(const Position&   position,
                                i32               ply,
                                Move              move,
                                i32               depth,
                                Score             score,
                                Score             static_eval,
                                tt_data::tt_bound bound) const -> void {
    tt_slot* slot = ptr(position);

    // If this position had a previous alpha raise, we can trust its move even if we did not raise
    // alpha again.
    if (!move && slot->key == position.hash()) {
        move = slot->data.move;
    }

    slot->key  = position.hash();
    slot->data = {
      .move        = move,
      .score       = score_to_tt(score, ply),
      .static_eval = static_cast<i16>(static_eval),
      .depth       = static_cast<i8>(depth),
      .bound       = bound,
    };
}

auto transposition_table::clear() const -> void {
    std::memset(m_data, 0, m_size * sizeof(*m_data));
}

auto transposition_table::allocate(usize mb) -> void {
    m_data = new tt_slot[mb_to_size(mb)];
    m_size = mb_to_size(mb);
    clear();
}

auto transposition_table::destroy() -> void {
    delete[] m_data;
    m_data = nullptr;
}

auto transposition_table::ptr(const Position& position) const -> tt_slot* {
    usize idx = position.hash() % m_size;
    return m_data + idx;
}
}  // namespace kerosene
