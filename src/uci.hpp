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

#include "position.hpp"
#include "repetition_table.hpp"
#include "search.hpp"
#include <string>

namespace kerosene {


class Uci {
public:
    auto loop() -> void;
    auto cli(int argc, char* argv[]) -> void;

private:
    auto execute_command(const std::string&) -> void;

    auto handle_position(std::istringstream& is) -> void;
    auto handle_d(std::istringstream& is) const -> void;
    auto handle_perft(std::istringstream& is) const -> void;
    auto handle_go(std::istringstream& is) -> void;
    auto handle_ucinewgame() -> void;
    auto handle_bench() -> void;

    Position        m_position{Position::parse(kStartPos)};
    RepetitionTable m_repetition_table{};

    searcher m_searcher{};
};

}  // kerosene
