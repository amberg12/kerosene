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

#include "dataset.hpp"
#include "sgdm.hpp"
#include <fstream>

using namespace kerosene;

auto main(int argc, char* argv[]) -> int {
    std::string  input_file = argv[1];
    std::fstream fs(input_file);

    tuning::Dataset dataset = tuning::parse_dataset(fs);

    tuning::sgdm(dataset, 500, 0.1, 32, 1e-4);
}
