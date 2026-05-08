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
#include "score.hpp"
#include "time_manager.hpp"

namespace kerosene {

enum class NodeType {
    kRoot,
    kNonPv,
};

class Searcher {
public:
    Searcher() = default;

    auto set_position(const Position& root_position) -> void;

    auto begin_search(TimeParameters time_parameters) -> void;

private:
    auto iterative_deepening() -> void;

    template<NodeType kNodeType>
    auto quiesce(const Position& position, Score alpha, Score beta, i32 ply) -> Score;

    template<NodeType kNodeType>
    auto search(const Position& position, i32 depth, Score alpha, Score beta, i32 ply) -> Score;

    Position    m_root_position = Position::parse(kStartPos);
    TimeManager m_time_manager;
    Move        m_best_move;
};

}  // namespace kerosene
