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

#include "move.hpp"
#include "move_generation.hpp"

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
    } else if (command == "position") {
        handle_position(is);
    } else if (command == "d") {
        handle_d(is);
    } else if (command == "perft") {
        handle_perft(is);
    }
}

auto Uci::handle_position(std::istringstream& is) -> void {
    std::string token, fen;

    while (is >> token) {
        if (token == "fen") {
            while (is >> token) {
                if (token == "moves") {
                    break;
                }

                fen += token + " ";
            }

            m_position = Position::parse(fen);
        } else if (token == "startpos") {
            m_position = Position::parse(kStartPos);
        } else if (token == "kiwipete") {
            m_position = Position::parse(kKiwiPete);
        }

        while (is >> token) {
            if (token == "moves") {
                continue;
            }

            Move move = Move::parse(token, m_position);
            m_position = Position{m_position, move};
        }
    }
}

auto Uci::handle_d(std::istringstream& is) const -> void {
    std::println("handling d");
    std::println("{}", m_position.to_string());
    std::println("Castling Rights: {}", m_position.castling_rights().to_string());
}

auto Uci::handle_perft(std::istringstream& is) const -> void {
    i32 depth;
    is >> depth;
    u64 total_nodes = perft(m_position, depth);
    std::println("Total nodes: {}", total_nodes);
}

}  // kerosene
