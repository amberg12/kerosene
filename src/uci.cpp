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

#include <iostream>
#include <print>
#include <sstream>

#include "common.hpp"
#include "uci.hpp"

namespace kerosene {


auto Uci::loop() -> void {
    std::string input;
    while (std::getline(std::cin, input)) {
        execute_command(input);
    }
}

auto Uci::cli(const int argc, char* argv[]) -> void {
    for (usize i = 1; i < argc; ++i) {
        execute_command(argv[i]);
    }
}

auto Uci::execute_command(const std::string& line) -> void {
    std::istringstream is{line};

    std::string command;
    is >> command;

    if (command == "uci") {
        std::println("id name Kerosene");
        std::println("id author Amber Goulding");
        std::println("uciok");
    } else if (command == "quit") {
        std::exit(0);
    }
}

}  // kerosene
