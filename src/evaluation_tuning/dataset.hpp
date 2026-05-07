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

#include <optional>
#include <string>
#include <vector>
#include "../util/integer_types.hpp"

namespace kerosene::tuning {

enum class Result : u8 {
    kBlack,
    kDraw,
    kWhite,
};

struct DatasetEntry {
    std::string fen;
    Result      result{};
};

using Dataset = std::vector<DatasetEntry>;

auto parse_dataset(std::istream& data, std::optional<usize> limit = std::nullopt) -> Dataset;

}
