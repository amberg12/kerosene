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

struct time_parameters {
    time::milliseconds wtime{};
    time::milliseconds btime{};
    time::milliseconds winc{};
    time::milliseconds binc{};
};

class time_manager {
public:
    time_manager() = default;
    time_manager(Color side_to_move, time_parameters time_parameters);

    [[nodiscard]] auto hard_stop() const -> bool;
    [[nodiscard]] auto soft_stop() const -> bool;

private:
    time::time_point   m_start_time{};
    time::milliseconds m_soft_limit{};
    time::milliseconds m_hard_limit{};
};

}  // namespace kerosene
