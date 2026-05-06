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

#pragma once
#include "types.hpp"
#include "util/integer_types.hpp"
namespace kerosene {

struct TimeParameters {
    time::Milliseconds wtime{};
    time::Milliseconds btime{};
    time::Milliseconds winc{};
    time::Milliseconds binc{};
};

class TimeManager {
public:
    TimeManager() = default;
    TimeManager(Color side_to_move, TimeParameters time_parameters);

    auto stop() const -> bool;

private:
    time::TimePoint    m_start_time{};
    time::Milliseconds m_time_limit{};
};

}  // namespace kerosene
