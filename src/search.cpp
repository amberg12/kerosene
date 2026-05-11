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

#include "search.hpp"

#include "evaluation.hpp"
#include "move_generation.hpp"
#include "move_picker.hpp"
#include "repetition_table.hpp"

namespace kerosene {
namespace {
constexpr i32 kMaxDepth = 255;

struct NodeInfo {
    bool is_root;
    bool is_pv;
};

template<NodeType kNodeType>
constexpr auto get_node_info() -> NodeInfo = delete;

template<>
constexpr auto get_node_info<NodeType::kRoot>() -> NodeInfo {
    return {.is_root = true, .is_pv = true};
}

template<>
constexpr auto get_node_info<NodeType::kNonPv>() -> NodeInfo {
    return {.is_root = false, .is_pv = false};
}
}

auto Searcher::set_position(const Position& root_position, const RepetitionTable& repetition_table)
  -> void {
    m_root_position    = root_position;
    m_repetition_table = repetition_table;
}

auto Searcher::begin_search(TimeParameters time_parameters) -> void {
    m_history = History{}; // TODO: remove - we would rather have a malus scheme over aging.

    m_time_manager = TimeManager{m_root_position.side_to_move(), time_parameters};
    iterative_deepening();
}

auto Searcher::new_game() -> void {
    m_tt.clear();
    m_history = History{};
}

auto Searcher::iterative_deepening() -> void {
    Move best_move = generate_legal_moves(m_root_position).front();

    for (i32 depth = 1; depth < kMaxDepth; ++depth) {
        Score score =
          search<NodeType::kRoot>(m_root_position, depth, kNegativeInf, kPositiveInf, 0);

        if (m_time_manager.stop()) {
            break;
        }

        best_move = m_best_move;

        std::string score_string = is_mate(score) ? "mate" : "cp";

        if (is_mate(score)) {
            score = (mate_in(score) + 1) / 2;
        }

        std::println("info depth {} score {} {}", depth, score_string, score);
    }

    std::println("bestmove {}", best_move.to_string());
}

template<NodeType kNodeType>
auto Searcher::quiesce(const Position& position, Score alpha, Score beta, i32 ply) -> Score {
    if (m_time_manager.stop()) {
        return 0;
    }

    if (m_repetition_table.is_repetition(position)) {
        return 0;
    }

    MovePicker mp{position, kNullMove, m_history};

    Score best_score           = kNegativeInf;
    i32   searched_legal_moves = 0;
    bool  skip_quiets          = false;

    if (position.checkers_nb() == 0) {
        skip_quiets = true;
        best_score  = evaluate(position);

        if (best_score >= beta) {
            return best_score;
        }

        if (best_score > alpha) {
            alpha = best_score;
        }
    }

    for (Move move = mp.next_move(skip_quiets); move; move = mp.next_move(skip_quiets)) {
        ++searched_legal_moves;

        Position child_position{position, move};
        m_repetition_table.push(child_position);

        Score    score = -quiesce<NodeType::kNonPv>(child_position, -beta, -alpha, ply + 1);

        m_repetition_table.pop();

        if (m_time_manager.stop()) {
            return 0;
        }

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                alpha = score;
            }
        }

        if (score >= beta) {
            break;
        }
    }

    if (searched_legal_moves == 0) {
        return position.checkers_nb() == 0 ? best_score : mated_in(ply);
    }

    return best_score;
}

template<NodeType kNodeType>
auto Searcher::search(const Position& position, i32 depth, Score alpha, Score beta, i32 ply)
  -> Score {
    if (m_time_manager.stop()) {
        return 0;
    }

    // We don't want to do draw detection at root if we have already repeated once.
    if (!get_node_info<kNodeType>().is_root && m_repetition_table.is_repetition(position)) {
        return 0;
    }

    if (depth <= 0) {
        return quiesce<kNodeType>(position, alpha, beta, ply);
    }

    std::optional<TData> tt = m_tt.probe(position);

    Move tt_move = get_node_info<kNodeType>().is_root ? m_best_move : tt ? tt->move : kNullMove;

    MovePicker mp{position, tt_move, m_history};

    Move  best_move            = kNullMove;
    Score best_score           = kNegativeInf;
    i32   searched_legal_moves = 0;

    for (Move move = mp.next_move(); move; move = mp.next_move()) {
        ++searched_legal_moves;

        bool is_loud = position.is_capture(move) || move.special_type() == Move::kPromotion;

        Position child_position{position, move};
        m_repetition_table.push(child_position);

        Score score = -search<NodeType::kNonPv>(child_position, depth - 1, -beta, -alpha, ply + 1);

        m_repetition_table.pop();

        if (m_time_manager.stop()) {
            return 0;
        }

        if (score > best_score) {
            best_score = score;
            best_move  = move;

            if (score > alpha) {
                alpha = score;
            }
        }

        if (score >= beta) {
            if (!is_loud) {
                m_history.update_quiet_history(position, depth, move);
            }

            break;
        }
    }

    if constexpr (get_node_info<kNodeType>().is_root) {
        m_best_move = best_move;
    }

    m_tt.write(position, best_move);

    if (searched_legal_moves == 0) {
        return position.checkers_nb() == 0 ? 0 : mated_in(ply);
    }

    return best_score;
}
}  // kerosene
