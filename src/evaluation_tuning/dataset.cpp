/*
  Shellac - A UCI chess engine.
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

#include "dataset.hpp"
#include <sstream>

namespace kerosene::tuning {

auto parse_dataset(std::istream& data, std::optional<usize> limit) -> Dataset {
    std::string line, token;
    Dataset     out;
    usize       lines = 0;

    while (std::getline(data, line)) {
        std::istringstream ss(line);

        DatasetEntry entry;

        while (ss >> token) {
            if (token == "[0.0]") {
                entry.result = Result::kBlack;
            } else if (token == "[0.5]") {
                entry.result = Result::kDraw;
            } else if (token == "[1.0]") {
                entry.result = Result::kWhite;
            } else {
                entry.fen += token + " ";
            }
        }

        out.emplace_back(entry);

        if (limit && *limit > lines) {
            break;
        }
    }

    return out;
}

}
