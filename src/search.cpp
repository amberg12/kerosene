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
#include "util/defer.hpp"
#include "util/math.hpp"

namespace kerosene {
namespace {
constexpr i32 kMaxDepth = 255;
}

auto Searcher::set_position(const Position& root_position, const RepetitionTable& repetition_table)
  -> void {
    m_root_position    = root_position;
    m_repetition_table = repetition_table;
}

auto Searcher::begin_search(TimeParameters time_parameters) -> void {
    m_nodes = 0;

    m_time_manager = TimeManager{m_root_position.side_to_move(), time_parameters};
    iterative_deepening();
}

auto Searcher::new_game() -> void {
    m_tt.clear();
    m_history   = History{};
    m_best_move = kNullMove;
}

auto Searcher::iterative_deepening() -> void {
    Score last_score = kScoreNone;
    i32   last_depth = -1;

    const auto print_info_line = [&] {
        std::string score_string = is_mate(last_score) ? "mate" : "cp";
        std::println("info depth {} score {} {}", last_depth, score_string, last_score);
    };

    MoveList emergency_moves = generate_legal_moves(m_root_position);
    m_best_move              = emergency_moves.front();

    for (i32 depth = 1; depth < kMaxDepth; ++depth) {
        Score alpha = kNegativeInf;
        Score beta  = kPositiveInf;
        Score delta = 25;
        Score score = kScoreNone;

        if (depth >= 5) {
            alpha = last_score - delta;
            beta  = last_score + delta;
        }

        while (true) {
            score = search<Root>(m_root_position, depth, alpha, beta, 0);

            if (score <= alpha) {
                alpha = std::max(alpha - delta, kNegativeInf);
            } else if (score >= beta) {
                beta = std::min(beta + delta, kPositiveInf);
            } else {
                break;
            }

            delta *= 2;

            if (m_time_manager.stop()) {
                break;
            }
        }

        if (m_time_manager.stop()) {
            break;
        }

        last_score = score;
        last_depth = depth;

        if (!m_time_manager.stop()) {
            print_info_line();
        }
    }

    print_info_line();
    std::println("bestmove {}", m_best_move.to_string());
}

template<typename Node>
auto Searcher::quiesce(const Position& position, Score alpha, Score beta, i32 ply) -> Score {
    m_nodes++;

    if (m_time_manager.stop()) {
        return 0;
    }

    if (m_repetition_table.is_repetition(position)) {
        return 0;
    }

    MovePicker mp{position, kNullMove, m_history, kNullMove};

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

        Score score = -quiesce<NonPv>(child_position, -beta, -alpha, ply + 1);

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

template<typename Node>
auto Searcher::search(const Position& position, i32 depth, Score alpha, Score beta, i32 ply)
  -> Score {
    m_nodes++;

    if (m_time_manager.stop()) {
        return 0;
    }

    // We don't want to do draw detection at root if we have already repeated once.
    if (!Node::is_root && m_repetition_table.is_repetition(position)) {
        return 0;
    }

    if (depth <= 0) {
        return quiesce<typename Node::Next>(position, alpha, beta, ply);
    }

    Stack& ss            = m_ss[ply];
    m_ss[ply + 1].killer = kNullMove;

    std::optional<TData> tt = m_tt.probe(position, ply);

    if (!Node::is_pv && tt && tt->depth >= depth && [&] {
        switch (tt->bound) {
        case TData::None:
            return false;
        case TData::Upper:
            return tt->score <= alpha;
        case TData::Lower:
            return tt->score >= beta;
        case TData::Exact:
            return true;
        }
    }()) {
        return tt->score;
    }

    Move tt_move = Node::is_root ? m_best_move : tt ? tt->move : kNullMove;

    Score static_eval = evaluate(position);

    if (!Node::is_pv && !position.check()) {
        if (depth <= 5 && static_eval - 150 * depth >= beta) {
            return static_eval;
        }
    }

    MoveList fail_low_quiets{};

    MovePicker mp{position, tt_move, m_history, ss.killer};

    Move  best_move  = kNullMove;
    Score best_score = kScoreNone;
    i32   move_count = 0;

    bool skip_quiets = false;
    for (Move move = mp.next_move(skip_quiets); move; move = mp.next_move(skip_quiets)) {
        bool is_loud = position.is_capture(move) || move.special_type() == Move::kPromotion;

        if (!Node::is_root && !position.check() && !is_loss(best_score)) {
            if (!is_loud && move_count >= 5 + depth * depth) {
                skip_quiets = true;
            }
        }

        ++move_count;

        Position child_position{position, move};
        m_repetition_table.push(child_position);
        DEFER {
            m_repetition_table.pop();
        };

        Score score{};

        i32 new_depth = depth - 1;

        if (depth >= 3 && move_count >= 3) {
            i32 r = 2 + log2(depth) * log2(move_count) / 4;

            const i32 lmr_depth = std::clamp(new_depth - r, 0, new_depth);

            score = -search<NonPv>(child_position, lmr_depth, -alpha - 1, -alpha, ply + 1);

            if (score > alpha && lmr_depth < new_depth) {
                score = -search<NonPv>(child_position, new_depth, -alpha - 1, -alpha, ply + 1);
            }
        } else if (!Node::is_pv || move_count > 1) {
            score = -search<NonPv>(child_position, new_depth, -alpha - 1, -alpha, ply + 1);
        }

        if (Node::is_pv && (move_count == 1 || score > alpha)) {
            score = -search<Pv>(child_position, new_depth, -beta, -alpha, ply + 1);
        }

        if (m_time_manager.stop()) {
            return 0;
        }

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;

                if constexpr (Node::is_root) {
                    m_best_move = best_move;
                }

                alpha = score;
            }
        }

        if (score >= beta) {
            if (!is_loud) {
                ss.add_killer(move);
                m_history.update_quiet_history(position, depth, move, fail_low_quiets);
            }

            break;
        }

        if (!is_loud && move != best_move) {
            fail_low_quiets.emplace_back(move);
        }
    }

    if (move_count == 0) {
        best_score = position.checkers_nb() == 0 ? 0 : mated_in(ply);
    }

    const TData::Bound bound = [&] {
        if (!best_move) {
            return TData::Bound::Upper;
        }

        if (best_score >= beta) {
            return TData::Bound::Lower;
        }

        return TData::Bound::Exact;
    }();

    m_tt.write(position, ply, best_move, depth, best_score, bound);

    return best_score;
}
}  // kerosene
