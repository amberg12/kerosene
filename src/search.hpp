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
#include "history.hpp"
#include "position.hpp"
#include "repetition_table.hpp"
#include "score.hpp"
#include "time_manager.hpp"
#include "transposition_table.hpp"

namespace kerosene {

using Nodes = u64;

class Searcher {
public:
    Searcher() = default;

    auto set_position(const Position& root_position, const RepetitionTable& repetition_table)
      -> void;

    auto begin_search(TimeParameters time_parameters) -> void;

    auto new_game() -> void;

    [[nodiscard]] auto nodes() const -> Nodes {
        return m_nodes;
    }

private:
    template<bool kIsPv, bool kIsRoot, typename N>
    struct NodeType {
        static constexpr bool is_pv   = kIsPv;
        static constexpr bool is_root = kIsRoot;

        using Next = N;
    };

    struct Root;
    struct Pv;
    struct NonPv;

    struct Root : NodeType<true, true, Pv> { };
    struct Pv : NodeType<true, false, Pv> { };
    struct NonPv : NodeType<false, false, NonPv> { };

    struct Stack {
        Move killer{};

        auto add_killer(Move move) -> void {
            killer = move;
        }
    };

    auto iterative_deepening() -> void;

    template<typename Node>
    auto quiesce(const Position& position, Score alpha, Score beta, i32 ply) -> Score;

    template<typename N>
    auto search(const Position& position, i32 depth, Score alpha, Score beta, i32 ply) -> Score;

    Nodes m_nodes{};

    Position               m_root_position = Position::parse(kStartPos);
    RepetitionTable        m_repetition_table{};
    TranspositionTable     m_tt{};
    History                m_history{};
    std::array<Stack, 256> m_ss{};

    TimeManager m_time_manager;
    Move        m_best_move;
};

}  // namespace kerosene
