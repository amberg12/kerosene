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
#include <ranges>

namespace kerosene {
namespace {
constexpr i32 max_depth              = 255;
constexpr u64 node_tm_check_interval = 4096;
}

auto searcher::set_position(const Position& root_position, const RepetitionTable& repetition_table)
  -> void {
    m_root_position    = root_position;
    m_repetition_table = repetition_table;
}

auto searcher::begin_search(time_parameters time_parameters) -> void {
    m_stopped    = false;
    m_nodes      = 0;
    m_node_table = multi_array_t<kerosene::nodes, Square::kNb, Square::kNb>{};

    m_time_manager = time_manager{m_root_position.side_to_move(), time_parameters};
    iterative_deepening();
}

auto searcher::new_game() -> void {
    m_tt.clear();
    m_history   = std::make_unique<history>();
    m_best_move = kNullMove;
}

auto searcher::iterative_deepening() -> void {
    Score last_score = kScoreNone;
    i32   last_depth = -1;
    line  last_pv{};

    const auto print_info_line = [&] {
        const std::string score_string = is_mate(last_score) ? "mate" : "cp";
        const std::string pv_string = [&] {
            namespace rv = std::views;
            std::string out;
            for (const Move move : last_pv | rv::take(last_pv.size() - 1)) {
                out += move.to_string() + " ";
            }

            out += last_pv.back().to_string();

            return out;
        }();

        std::println("info depth {} score {} {} pv {}", last_depth, score_string, last_score, pv_string);
    };

    MoveList emergency_moves = generate_legal_moves(m_root_position);
    m_best_move              = emergency_moves.front();

    for (i32 depth = 1; depth < max_depth; ++depth) {
        line  pv{};
        Score alpha = kNegativeInf;
        Score beta  = kPositiveInf;
        Score delta = 25;
        Score score = kScoreNone;

        if (depth >= 5) {
            alpha = last_score - delta;
            beta  = last_score + delta;
        }

        while (true) {
            score = search<root_node>(m_root_position, depth, alpha, beta, 0, pv);

            if (score <= alpha) {
                alpha = std::max(alpha - delta, kNegativeInf);
            } else if (score >= beta) {
                beta = std::min(beta + delta, kPositiveInf);
            } else {
                break;
            }

            delta *= 2;

            if (m_stopped) {
                break;
            }
        }

        if (m_stopped) {
            break;
        }

        last_score = score;
        last_depth = depth;
        last_pv    = pv;

        Move best_move = last_pv[0];

        // Only touch the soft limit above depth 5 to avoid instability found in low depths.
        if (depth >= 5) {
            const f64 node_ratio =
              static_cast<f64>(m_node_table[best_move.src()][best_move.dst()])
              / static_cast<f64>(m_nodes);
            m_time_manager.recompute_soft_limit(node_ratio);
        }

        if (m_time_manager.soft_stop()) {
            break;
        }

        print_info_line();
    }

    print_info_line();
    std::println("bestmove {}", last_pv[0].to_string());
}

template<typename Node>
auto searcher::quiesce(const Position& position, Score alpha, Score beta, i32 ply, line& pv)
  -> Score {
    m_nodes++;

    if (m_stopped || (m_nodes % node_tm_check_interval == 0 && m_time_manager.hard_stop())) {
        m_stopped = true;
        return 0;
    }

    if (m_repetition_table.is_repetition(position) || position.fifty_move_rule()) {
        return 0;
    }

    MovePicker mp{position, kNullMove, *m_history, m_ss, ply, kNullMove};

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

        line child_pv{};

        Score score = -quiesce<non_pv_node>(child_position, -beta, -alpha, ply + 1, child_pv);

        m_repetition_table.pop();

        if (m_stopped) {
            return 0;
        }

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                alpha = score;

                if constexpr (Node::is_pv) {
                    pv.clear();
                    pv.emplace_back(move);

                    for (const Move pv_move : child_pv) {
                        pv.emplace_back(pv_move);
                    }
                }
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
auto searcher::search(
  const Position& position, i32 depth, Score alpha, Score beta, i32 ply, line& pv) -> Score {
    m_nodes++;

    if (m_stopped || (m_nodes % node_tm_check_interval == 0 && m_time_manager.hard_stop())) {
        m_stopped = true;
        return 0;
    }

    // We don't want to do draw detection at root if we have already repeated once.
    if (!Node::is_root
        && (m_repetition_table.is_repetition(position) || position.fifty_move_rule())) {
        return 0;
    }

    if (depth <= 0) {
        return quiesce<typename Node::next>(position, alpha, beta, ply, pv);
    }

    ss_item& ss                  = m_ss.at(ply);
    m_ss.at(ply + 1).killer_move = kNullMove;

    std::optional<tt_data> tt = m_tt.probe(position, ply);

    if (!Node::is_pv && tt && tt->depth >= depth && [&] {
            switch (tt->bound) {
            case tt_data::none:
                return false;
            case tt_data::upper:
                return tt->score <= alpha;
            case tt_data::lower:
                return tt->score >= beta;
            case tt_data::exact:
                return true;
            }
        }()) {
        return tt->score;
    }

    Move tt_move = Node::is_root ? m_best_move : tt ? tt->move : kNullMove;

    Score static_eval = tt ? tt->static_eval : evaluate(position);

    if (!Node::is_pv && !position.check()) {
        if (depth <= 5 && static_eval - 150 * depth >= beta) {
            return static_eval;
        }
    }

    MoveList fail_low_quiets{};

    MovePicker mp{position, tt_move, *m_history, m_ss, ply, ss.killer_move};

    Move  best_move  = kNullMove;
    Score best_score = kScoreNone;
    i32   move_count = 0;

    bool skip_quiets = false;
    for (Move move = mp.next_move(skip_quiets); move; move = mp.next_move(skip_quiets)) {
        bool is_loud = position.is_capture(move) || move.special_type() == Move::kPromotion;

        nodes nodes_before_search = m_nodes;

        if (!Node::is_root && !position.check() && !is_loss(best_score)) {
            if (!is_loud && move_count >= 5 + depth * depth) {
                skip_quiets = true;
            }

            if (!is_loud && depth <= 3 && static_eval + 256 + 128 * depth <= alpha) {
                skip_quiets = true;
            }
        }

        ++move_count;

        Position child_position{position, move};

        ss.conthist_subtable = &m_history->cont_history.read(position, move);
        DEFER {
            ss.conthist_subtable = nullptr;
        };

        m_repetition_table.push(child_position);
        DEFER {
            m_repetition_table.pop();
        };

        Score score{};

        i32  new_depth = depth - 1;
        line child_pv{};

        if (depth >= 3 && move_count >= 3) {
            i32 r = 2 + log2(depth) * log2(move_count) / 4;

            const i32 lmr_depth = std::clamp(new_depth - r, 0, new_depth);

            score = -search<non_pv_node>(child_position, lmr_depth, -alpha - 1, -alpha, ply + 1,
                                         child_pv);

            if (score > alpha && lmr_depth < new_depth) {
                score = -search<non_pv_node>(child_position, new_depth, -alpha - 1, -alpha, ply + 1,
                                             child_pv);
            }
        } else if (!Node::is_pv || move_count > 1) {
            score = -search<non_pv_node>(child_position, new_depth, -alpha - 1, -alpha, ply + 1,
                                         child_pv);
        }

        if (Node::is_pv && (move_count == 1 || score > alpha)) {
            score = -search<pv_node>(child_position, new_depth, -beta, -alpha, ply + 1, child_pv);
        }

        if (m_stopped) {
            return 0;
        }

        nodes nodes_after_search = m_nodes;

        if constexpr (Node::is_root) {
            m_node_table[move.src()][move.dst()] += nodes_after_search - nodes_before_search;
        }

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;

                if constexpr (Node::is_root) {
                    m_best_move = move;
                }

                if constexpr (Node::is_pv) {
                    pv.clear();
                    pv.emplace_back(move);

                    for (const Move pv_move : child_pv) {
                        pv.emplace_back(pv_move);
                    }
                }

                alpha = score;
            }
        }

        if (score >= beta) {
            if (!is_loud) {
                ss.killer_move = move;

                m_history->quiet_history.write(position, best_move, bonus(depth));

                for (const Move fail_low : fail_low_quiets) {
                    m_history->quiet_history.write(position, fail_low, malus(depth));
                }

                for (const i32 ply_offset : conthist_offsets) {
                    const ss_item& p_ss = m_ss.at(ply - ply_offset);

                    if (p_ss.conthist_subtable) {
                        p_ss.conthist_subtable->write(position, best_move, bonus(depth));

                        for (const Move fail_low : fail_low_quiets) {
                            p_ss.conthist_subtable->write(position, fail_low, malus(depth));
                        }
                    }
                }
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

    const tt_data::tt_bound bound = [&] {
        if (!best_move) {
            return tt_data::tt_bound::upper;
        }

        if (best_score >= beta) {
            return tt_data::tt_bound::lower;
        }

        return tt_data::tt_bound::exact;
    }();

    m_tt.write(position, ply, best_move, depth, best_score, static_eval, bound);

    return best_score;
}
}  // kerosene
