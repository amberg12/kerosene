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
#include "search_stack.hpp"
#include "time_manager.hpp"
#include "transposition_table.hpp"
#include "util/inplace_vector.hpp"
#include "util/multi_array.hpp"

namespace kerosene {

using nodes = u64;
using line = inplace_vector<Move, 512>;

class searcher {
public:
    searcher() = default;

    auto set_position(const Position& root_position, const RepetitionTable& repetition_table)
      -> void;

    auto begin_search(time_parameters time_parameters) -> void;

    auto new_game() -> void;

    [[nodiscard]] auto node_count() const -> nodes {
        return m_nodes;
    }

private:
    template<bool pv, bool root, typename N>
    struct node_type {
        static constexpr bool is_pv   = pv;
        static constexpr bool is_root = root;

        using next = N;
    };

    struct root_node;
    struct pv_node;
    struct non_pv_node;

    struct root_node : node_type<true, true, pv_node> { };
    struct pv_node : node_type<true, false, pv_node> { };
    struct non_pv_node : node_type<false, false, non_pv_node> { };

    struct Stack {
        Move killer{};

        auto add_killer(Move move) -> void {
            killer = move;
        }
    };

    auto iterative_deepening() -> void;

    template<typename Node>
    auto quiesce(const Position& position, Score alpha, Score beta, i32 ply, line& pv_line) -> Score;

    template<typename N>
    auto search(const Position& position, i32 depth, Score alpha, Score beta, i32 ply, line& pv_line) -> Score;

    nodes m_nodes{};

    multi_array_t<nodes, Square::kNb, Square::kNb> m_node_table{};

    Position                 m_root_position = Position::parse(kStartPos);
    RepetitionTable          m_repetition_table{};
    transposition_table       m_tt{};
    std::unique_ptr<history> m_history{};
    search_stack             m_ss{};

    time_manager m_time_manager;
    Move         m_best_move;

    bool m_stopped{};
};

}  // namespace kerosene
