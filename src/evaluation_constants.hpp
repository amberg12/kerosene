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
#include "score.hpp"

namespace kerosene::evaluation_constants {
constexpr ScorePair kPawnMaterial   = ScorePair{110, 670};
constexpr ScorePair kKnightMaterial = ScorePair{423, 1566};
constexpr ScorePair kBishopMaterial = ScorePair{463, 1696};
constexpr ScorePair kRookMaterial   = ScorePair{504, 2741};
constexpr ScorePair kQueenMaterial  = ScorePair{1883, 3900};
}  // namespace kerosene::evaluation_constants
