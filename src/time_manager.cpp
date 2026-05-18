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

constexpr time::milliseconds uci_margin{50};

time_manager::time_manager(Color side_to_move, time_parameters time_parameters) {
    using namespace std::chrono_literals;

    auto [time, inc] = side_to_move == Color::kWhite
                       ? std::pair{time_parameters.wtime, time_parameters.winc}
                       : std::pair{time_parameters.btime, time_parameters.binc};

    m_start_time = time::clock::now();

    time::milliseconds safe_time = std::max(time - uci_margin, 0ms);

    m_start_time = time::clock::now();
    m_time_limit = safe_time / 20 + inc / 2;
}

auto time_manager::stop() const -> bool {
    using namespace std::chrono_literals;

    return time::clock::now() > m_start_time + m_time_limit;
}

}
