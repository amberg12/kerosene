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

#include "../position.hpp"
#include "../util/integer_types.hpp"
#include "evaluation_trace.hpp"
#include <optional>
#include <utility>
#include <vector>

namespace kerosene::tuning {

enum class Result : u8 {
    kBlack,
    kDraw,
    kWhite,
};

constexpr auto parse(Result r) -> f64 {
    switch (r) {
    case Result::kBlack:
        return 0.0;
    case Result::kDraw:
        return 0.5;
    case Result::kWhite:
        return 1.0;
    }

    std::unreachable();
}

struct DatasetEntry {
    EvaluationTrace X_i;
    i32             phase{};
    Result          result{};
};

using Dataset = std::vector<DatasetEntry>;

auto parse_dataset(std::istream& data, std::optional<usize> limit = std::nullopt) -> Dataset;

}
