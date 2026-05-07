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
#include "util/integer_types.hpp"

namespace kerosene {

using Score = i32;

constexpr Score kNegativeInf = -32'000;
constexpr Score kPositiveInf = 32'000;

constexpr Score kMateScore     = 31'000;
constexpr Score kMateThreshold = 30'000;

constexpr auto mated_in(i32 ply) -> Score {
    return -kMateScore + ply;
}

constexpr auto is_mate(Score score) -> bool {
    return std::abs(score) > kMateThreshold;
}

constexpr auto mate_in(Score score) -> i32 {
    return score > 0 ? kMateScore - score : kMateScore + score;
}

struct ScorePair {
    Score mg{};
    Score eg{};

    [[nodiscard]] auto taper(i32 phase) const -> Score {
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    friend auto operator+=(ScorePair& lhs, ScorePair rhs) -> ScorePair& {
        lhs.mg += rhs.mg;
        lhs.eg += rhs.eg;
        return lhs;
    }

    friend auto operator-=(ScorePair& lhs, ScorePair rhs) -> ScorePair& {
        lhs.mg -= rhs.mg;
        lhs.eg -= rhs.eg;
        return lhs;
    }
};

}  // namespace kerosene