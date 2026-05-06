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

#include "time_manager.hpp"

namespace kerosene {

TimeManager::TimeManager(Color side_to_move, TimeParameters time_parameters) {
    auto [time, inc] = side_to_move == Color::kWhite
                       ? std::pair{time_parameters.wtime, time_parameters.winc}
                       : std::pair{time_parameters.btime, time_parameters.binc};

    m_start_time = time::Clock::now();
    m_time_limit = time / 20 + inc / 2;
}

auto TimeManager::stop() const -> bool {
    return time::Clock::now() > m_start_time + m_time_limit;
}

}
