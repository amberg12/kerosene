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

#include "move_generation.hpp"

namespace kerosene {
namespace {
auto is_pin_safe(const Position& pos, Square src, Square dst) -> bool {
    if (pos.pin_rays().has_square(src)) {
        bool aligned_with_king =
          dst.diagonal_to(pos.king_square()) || dst.orthogonal_to(pos.king_square());

        if (!aligned_with_king) {
            return false;
        }

        auto king_to_src = BitBoard::ray_inclusive(src, pos.king_square());
        auto king_to_dst = BitBoard::ray_inclusive(dst, pos.king_square());

        bool resolves_pin = king_to_dst.has_square(src) || king_to_src.has_square(dst);

        if (!resolves_pin) {
            return false;
        }
    }

    return true;
}

auto generate_pawn_moves(const Position& pos,
                         Color           side_to_move,
                         MoveList&       move_list,
                         BitBoard        allowed_squares) -> void {
    PieceMask pawn_ids = pos.piece_mask_for(side_to_move, PieceType::kPawn);

    Direction push_direction = Direction::pawn_direction(side_to_move);

    for (PieceId pawn_id : pawn_ids) {
        Square src;
        std::tie(src, std::ignore) = pos.info_of(pawn_id, side_to_move);

        Square single_dst = src + push_direction;

        if (!is_pin_safe(pos, src, single_dst)) {
            continue;
        }

        if (pos.piece_at(single_dst).empty()) {
            if (single_dst.promotion_rank(side_to_move)) {
                if (allowed_squares.has_square(single_dst)) {
                    move_list.emplace_back(
                      Move::create_promotion(src, single_dst, PieceType::kKnight));
                    move_list.emplace_back(
                      Move::create_promotion(src, single_dst, PieceType::kBishop));
                    move_list.emplace_back(
                      Move::create_promotion(src, single_dst, PieceType::kRook));
                    move_list.emplace_back(
                      Move::create_promotion(src, single_dst, PieceType::kQueen));
                }
            } else {
                if (allowed_squares.has_square(single_dst)) {
                    move_list.emplace_back(src, single_dst);
                }

                Square double_dst = single_dst + push_direction;

                if (single_dst.third_rank(side_to_move) && pos.piece_at(double_dst).empty()
                    && allowed_squares.has_square(double_dst)) {
                    move_list.emplace_back(src, double_dst);
                }
            }
        }
    }
}

auto generate_en_passant(const Position& pos,
                         Color           side_to_move,
                         MoveList&       move_list,
                         BitBoard        allowed_squares) -> void {
    if (pos.en_passant() == Square::kInvalid) {
        return;
    }

    Square    en_passant_victim_location{pos.en_passant().file(),
                                      side_to_move == Color::kWhite ? 4 : 3};
    PieceMask attackers = pos.attackers_to(pos.en_passant())[side_to_move];

    PieceId   en_passant_target_id = pos.tile_at(en_passant_victim_location).unpack().first;
    PieceMask king_attackers       = pos.attackers_to(pos.king_square())[~side_to_move];

    if (king_attackers.popcount() != 0 && !king_attackers.has_id(en_passant_target_id)) {
        return;
    }

    for (PieceId piece_id : attackers) {
        auto [src, piece_type] = pos.info_of(piece_id, side_to_move);

        if (piece_type != PieceType::kPawn) {
            continue;
        }

        BitBoard pieces_after_ep = pos.pieces() ^ BitBoard { en_passant_victim_location }
                                 ^ BitBoard { src } ^ BitBoard { pos.en_passant() };

        Square king_square = pos.king_square();
        bool   exposed     = false;

        constexpr std::array<std::pair<i8, i8>, 4> kOrthogonal = {
          {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        constexpr std::array<std::pair<i8, i8>, 4> kDiagonal = {
          {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}};

        auto scan = [&](std::span<const std::pair<i8, i8>> directions, auto is_threatening) {
            for (auto [d_file, d_rank] : directions) {
                for (i8 file = static_cast<i8>(king_square.file() + d_file),
                        rank = static_cast<i8>(king_square.rank() + d_rank);
                     file >= 0 && file <= 7 && rank >= 0 && rank <= 7;
                     file = static_cast<i8>(file + d_file), rank = static_cast<i8>(rank + d_rank)) {
                    Square square{file, rank};

                    if (!pieces_after_ep.has_square(square)) {
                        continue;
                    }

                    if (square != pos.en_passant()) {
                        Piece piece = pos.piece_at(square);
                        if (piece.color() == ~side_to_move && is_threatening(piece.piece_type())) {
                            exposed = true;
                        }
                    }

                    break;
                }
            }
        };

        scan(kOrthogonal, [](PieceType piece_type) {
            return piece_type == PieceType::kRook || piece_type == PieceType::kQueen;
        });

        scan(kDiagonal, [](PieceType piece_type) {
            return piece_type == PieceType::kBishop || piece_type == PieceType::kQueen;
        });

        if (!exposed) {
            move_list.emplace_back(Move::create_en_passant(src, pos.en_passant()));
        }
    }
}

auto generate_moves_to(const Position& pos,
                       Square          dst,
                       Color           side_to_move,
                       MoveList&       move_list,
                       BitBoard        allowed_squares) -> void {
    if (!allowed_squares.has_square(dst)) {
        return;
    }

    Piece attacked_piece = pos.piece_at(dst);
    if (!attacked_piece.empty() && attacked_piece.color() == side_to_move) {
        return;
    }

    PieceMask attackers = pos.attackers_to(dst)[side_to_move];

    for (PieceId id : attackers) {
        if (id == PieceId::king()) {
            continue;
        }

        auto [src, piece_type] = pos.info_of(id, side_to_move);

        if (!is_pin_safe(pos, src, dst)) {
            continue;
        }

        if (piece_type != PieceType::kPawn || !pos.piece_at(dst).empty()) {
            if (piece_type == PieceType::kPawn && dst.promotion_rank(side_to_move)) {
                move_list.emplace_back(Move::create_promotion(src, dst, PieceType::kKnight));
                move_list.emplace_back(Move::create_promotion(src, dst, PieceType::kBishop));
                move_list.emplace_back(Move::create_promotion(src, dst, PieceType::kRook));
                move_list.emplace_back(Move::create_promotion(src, dst, PieceType::kQueen));
            } else {
                move_list.emplace_back(src, dst);
            }
        }
    }
}

auto generate_king_moves(const Position& pos, Color side_to_move, MoveList& move_list) -> void {
    BitBoard  king_destinations = pos.attacked_by(side_to_move, PieceId::king());
    PieceMask king_attackers    = pos.attackers_to(pos.king_square())[~side_to_move];

    for (const Square dst : king_destinations) {
        if (!pos.piece_at(dst).empty() && pos.piece_at(dst).color() == side_to_move) {
            continue;
        }

        PieceMask attackers_to_dst = pos.attackers_to(dst)[~side_to_move];

        if (attackers_to_dst.popcount() != 0) {
            continue;
        }

        PieceId id_at_dst;
        std::tie(id_at_dst, std::ignore) = pos.tile_at(dst).unpack();

        bool dst_attacked_through_king = false;

        for (PieceId attacker_id : king_attackers) {
            if (attacker_id == id_at_dst) {
                continue;
            }

            auto [attacker_square, attacker_piece_type] = pos.info_of(attacker_id, ~side_to_move);

            if ((attacker_piece_type == PieceType::kRook
                 || attacker_piece_type == PieceType::kQueen)
                && attacker_square.orthogonal_to(dst)) {
                if (!BitBoard::ray_exclusive(dst, attacker_square)
                       .intersects(pos.pieces() & ~pos.pieces(side_to_move, PieceType::kKing))) {
                    dst_attacked_through_king = true;
                }
            }

            if ((attacker_piece_type == PieceType::kBishop
                 || attacker_piece_type == PieceType::kQueen)
                && attacker_square.diagonal_to(dst)) {
                if (!BitBoard::ray_exclusive(dst, attacker_square)
                       .intersects(pos.pieces() & ~pos.pieces(side_to_move, PieceType::kKing))) {
                    dst_attacked_through_king = true;
                }
            }
        }

        if (!dst_attacked_through_king) {
            move_list.emplace_back(pos.king_square(), dst);
        }
    }
}

auto generate_moves_to_one_checker(const Position& pos,
                                   Square          dst,
                                   Color           side_to_move,
                                   MoveList&       move_list,
                                   BitBoard        allowed_squares) -> void {

    return generate_moves_to(pos, dst, side_to_move, move_list, allowed_squares);
}

auto generate_castling(const Position& pos, Color side_to_move, MoveList& move_list) -> void {
    if (side_to_move == Color::kWhite) {
        if (pos.castling_rights().has_castling_rights(CastlingRights::kWhiteKSide)) {
            for (Square square : kWhiteKSideSquares) {
                if (!pos.piece_at(square).empty()) {
                    goto white_qside;
                }

                if (pos.attackers_to(square)[~side_to_move].popcount() != 0) {
                    goto white_qside;
                }
            }

            move_list.emplace_back(Move::create_castling(Square::kE1, Square::kG1));
        }

white_qside:
        if (pos.castling_rights().has_castling_rights(CastlingRights::kWhiteQSide)) {
            for (Square square : kWhiteQSideSquares) {
                if (!pos.piece_at(square).empty()) {
                    return;
                }

                if (pos.attackers_to(square)[~side_to_move].popcount() != 0) {
                    return;
                }
            }

            if (!pos.piece_at(Square::kB1).empty()) {
                return;
            }

            move_list.emplace_back(Move::create_castling(Square::kE1, Square::kC1));
        }
    } else {
        if (pos.castling_rights().has_castling_rights(CastlingRights::kBlackKSide)) {
            for (Square square : kBlackKSideSquares) {
                if (!pos.piece_at(square).empty()) {
                    goto black_qside;
                }

                if (pos.attackers_to(square)[~side_to_move].popcount() != 0) {
                    goto black_qside;
                }
            }

            move_list.emplace_back(Move::create_castling(Square::kE8, Square::kG8));
        }

black_qside:
        if (pos.castling_rights().has_castling_rights(CastlingRights::kBlackQSide)) {
            for (Square square : kBlackQSideSquares) {
                if (!pos.piece_at(square).empty()) {
                    return;
                }

                if (pos.attackers_to(square)[~side_to_move].popcount() != 0) {
                    return;
                }
            }

            if (!pos.piece_at(Square::kB8).empty()) {
                return;
            }

            move_list.emplace_back(Move::create_castling(Square::kE8, Square::kC8));
        }
    }
}

auto generate_legal_moves_zero_checkers(const Position& pos) -> MoveList {
    MoveList out{};
    Color    side_to_move = pos.side_to_move();

    for (Square square : kSquares) {
        generate_moves_to(pos, square, side_to_move, out, BitBoard::full());
    }

    generate_pawn_moves(pos, side_to_move, out, BitBoard::full());
    generate_en_passant(pos, side_to_move, out, BitBoard::full());
    generate_king_moves(pos, side_to_move, out);
    generate_castling(pos, side_to_move, out);

    return out;
}

auto generate_legal_moves_one_checker(const Position& pos) -> MoveList {
    MoveList out{};
    Color    side_to_move = pos.side_to_move();

    PieceId king_attacker = *pos.attackers_to(pos.king_square())[~pos.side_to_move()].begin();
    auto [attacker_square, attacker_piece_type] = pos.info_of(king_attacker, ~pos.side_to_move());

    BitBoard allowed_squares{};
    allowed_squares.set_square(attacker_square);
    if (attacker_piece_type.slider()) {
        allowed_squares |= BitBoard::ray_inclusive(attacker_square, pos.king_square());
    }

    for (Square square : kSquares) {
        generate_moves_to_one_checker(pos, square, side_to_move, out, allowed_squares);
    }

    generate_pawn_moves(pos, side_to_move, out, allowed_squares);
    generate_en_passant(pos, side_to_move, out, allowed_squares);
    generate_king_moves(pos, side_to_move, out);

    return out;
}

auto generate_legal_moves_two_checkers(const Position& pos) -> MoveList {
    MoveList out{};
    generate_king_moves(pos, pos.side_to_move(), out);
    return out;
}
}

auto generate_legal_moves(const Position& pos) -> MoveList {
    i32 checkers_nb = pos.checkers_nb();

    if (checkers_nb == 0) {
        return generate_legal_moves_zero_checkers(pos);
    } else if (checkers_nb == 1) {
        return generate_legal_moves_one_checker(pos);
    } else {
        return generate_legal_moves_two_checkers(pos);
    }
}
}  // kerosene
