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

auto get_base_soft_limit(time::milliseconds safe_time, time::milliseconds inc)
  -> time::milliseconds {
    return safe_time / 20 + inc / 2;
}

time_manager::time_manager(Color side_to_move, time_parameters time_parameters) {
    using namespace std::chrono_literals;

    auto [time, inc] = side_to_move == Color::kWhite
                       ? std::pair{time_parameters.wtime, time_parameters.winc}
                       : std::pair{time_parameters.btime, time_parameters.binc};

    m_inc       = inc;
    m_safe_time = std::max(time - uci_margin, 0ms);

    m_start_time = time::clock::now();

    m_soft_limit = get_base_soft_limit(m_safe_time, m_inc);
    m_hard_limit = m_safe_time / 3 + m_inc * 9 / 10;
}

auto time_manager::hard_stop() const -> bool {
    return time::clock::now() > m_start_time + m_hard_limit;
}

auto time_manager::soft_stop() const -> bool {
    return time::clock::now() > m_start_time + m_soft_limit;
}

auto time_manager::recompute_soft_limit(f64 node_ratio) -> void {
    using namespace std::chrono_literals;

    m_soft_limit = get_base_soft_limit(m_safe_time, m_inc);

    const f64 node_ratio_factor = 1.5 - node_ratio;

    m_soft_limit = std::clamp(time::cast<time::milliseconds>(m_soft_limit * node_ratio_factor), 0ms,
                              m_hard_limit);
}

}
