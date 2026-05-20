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

#include <sstream>
#include <string>

#include "position.hpp"

namespace kerosene {

auto PieceList::add_piece(Square at, Piece to) -> PieceId {
    PieceId piece_id = [&] {
        if (to.piece_type() == PieceType::kKing) {
            return PieceId::king();
        }

        for (PieceId id : ~m_piece_mask) {
            if (id == PieceId::king()) {
                continue;
            }

            return id;
        }

        std::unreachable();
    }();

    m_squares[static_cast<usize>(piece_id)]     = at;
    m_piece_types[static_cast<usize>(piece_id)] = to.piece_type();
    m_piece_mask.set_id(piece_id);

    return piece_id;
}

auto PieceList::move_piece(PieceId piece_id, Square dst) -> void {
    m_squares[static_cast<usize>(piece_id)] = dst;
}

auto PieceList::delete_piece(PieceId piece_id) -> void {
    m_piece_mask.unset_id(piece_id);
}

auto PieceList::mutate_piece(PieceId piece_id, PieceType to) -> void {
    m_piece_types[static_cast<usize>(piece_id)] = to;
}

Position::Position(const Position& parent, Move move) :
    m_mail_box(parent.m_mail_box),
    m_piece_list(parent.m_piece_list),
    m_bit_boards(parent.m_bit_boards),
    m_key(parent.m_key),
    m_side_to_move(parent.m_side_to_move),
    m_move_rule(parent.m_move_rule),
    m_castling_rights(parent.m_castling_rights),
    m_en_passant_target_square(parent.m_en_passant_target_square) {
    make_move(move);
    lazy_generate_attack_table();
    calculate_pin_rays();
}

auto Position::parse(std::string_view fen) -> Position {
    Position out{};

    std::string        input{fen};
    std::istringstream is{input};

    std::string board, color, castling_rights, en_passant, move_rule;
    is >> board >> color >> castling_rights >> en_passant >> move_rule;

    u8 file = 0, rank = 7;
    for (char c : board) {
        if (c == '/') {
            --rank;
            file = 0;
            continue;
        }

        if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Piece  to = Piece::parse(c);
            Square at = Square{file, rank};

            out.add_piece(at, to);

            ++file;
        }
    }

    out.m_side_to_move = Color::parse(color);

    if (out.m_side_to_move == Color::kBlack) {
        out.m_key ^= z_key_side_to_move();
    }

    for (char c : castling_rights) {
        switch (c) {
        case 'K':
            out.m_castling_rights.add_casting_rights(CastlingRights::kWhiteKSide);
            break;
        case 'k':
            out.m_castling_rights.add_casting_rights(CastlingRights::kBlackKSide);
            break;
        case 'Q':
            out.m_castling_rights.add_casting_rights(CastlingRights::kWhiteQSide);
            break;
        case 'q':
            out.m_castling_rights.add_casting_rights(CastlingRights::kBlackQSide);
            break;
        default:
            break;
        }
    }

    out.m_key ^= z_key_castling_rights(out.m_castling_rights);

    out.m_en_passant_target_square = Square::parse(en_passant).value_or(Square::kInvalid);

    if (out.m_en_passant_target_square != Square::kInvalid) {
        out.m_key ^= z_key_en_passant_file(out.m_en_passant_target_square.file());
    }

    out.m_move_rule = std::stoi(move_rule);

    out.lazy_generate_attack_table();
    out.calculate_pin_rays();

    return out;
}

auto Position::piece_at(Square at) const -> Piece {
    return tile_at(at).unpack().second;
}

auto Position::tile_at(Square at) const -> Tile {
    return m_mail_box[at];
}

auto Position::attackers_to(Square to) const -> ColorMap<PieceMask> {
    PieceMask white = m_attack_table[Color::kWhite][to];
    PieceMask black = m_attack_table[Color::kBlack][to];

    return {white, black};
}

auto Position::info_of(PieceId id, Color color) const -> std::pair<Square, PieceType> {
    return m_piece_list[color].get_info(id);
}

auto Position::to_string() const -> std::string {
    std::string out;

    for (u8 rank = 8; rank-- > 0;) {
        out += "+-+-+-+-+-+-+-+-+\n";
        for (u8 file = 0; file < 8; ++file) {
            Square at{file, rank};

            out += "|";
            out += piece_at(at).to_string();
        }
        out += "|\n";
    }

    out += "+-+-+-+-+-+-+-+-+";

    return out;
}

auto Position::castling_rights() const -> const CastlingRights& {
    return m_castling_rights;
}

auto Position::en_passant() const -> Square {
    return m_en_passant_target_square;
}

auto Position::side_to_move() const -> Color {
    return m_side_to_move;
}

auto Position::piece_mask_for(Color side_to_move, PieceType piece_type) const -> PieceMask {
    return m_piece_list[side_to_move].piece_type(piece_type);
}

auto Position::piece_count(Color side_to_move, PieceType piece_type) const -> i32 {
    return piece_mask_for(side_to_move, piece_type).popcount();
}

auto Position::pieces(Color color) const -> BitBoard {
    return pieces(color, PieceType::kPawn, PieceType::kKnight, PieceType::kBishop, PieceType::kRook,
                  PieceType::kQueen, PieceType::kKing);
}

auto Position::pieces() const -> BitBoard {
    return pieces(Color::kWhite) | pieces(Color::kBlack);
}

auto Position::phase() const -> i32 {
    i32 out{};

    out += piece_count(Color::kWhite, PieceType::kKnight) * 1;
    out += piece_count(Color::kBlack, PieceType::kKnight) * 1;

    out += piece_count(Color::kWhite, PieceType::kBishop) * 1;
    out += piece_count(Color::kBlack, PieceType::kBishop) * 1;

    out += piece_count(Color::kWhite, PieceType::kRook) * 2;
    out += piece_count(Color::kBlack, PieceType::kRook) * 2;

    out += piece_count(Color::kWhite, PieceType::kQueen) * 4;
    out += piece_count(Color::kBlack, PieceType::kQueen) * 4;

    return std::clamp(out, 0, 24);
}

auto Position::attacked_by(Color color, PieceId id) const -> BitBoard {
    return m_attack_table[color].attacked_by(id);
}

auto Position::mobility_of(Color side_to_move, PieceId piece_id) const -> i32 {
    return m_attack_table[side_to_move].mobility_of(piece_id);
}

auto Position::king_square(Color side_to_move) const -> Square {
    auto [square, piece_type] = info_of(PieceId::king(), side_to_move);
    return square;
}

auto Position::king_square() const -> Square {
    return king_square(side_to_move());
}

auto Position::checkers_nb() const -> i32 {
    Square    king_square  = Position::king_square();
    PieceMask checker_mask = attackers_to(king_square)[~m_side_to_move];
    return checker_mask.popcount();
}

auto Position::check() const -> bool {
    return checkers_nb() > 0;
}

auto Position::pin_rays() const -> BitBoard {
    return m_pin_rays;
}

auto Position::is_capture(Move move) const -> bool {
    return move.special_type() == Move::kEnPassant
        || (!piece_at(move.dst()).empty() && piece_at(move.dst()).color() != m_side_to_move);
}

auto Position::hash() const -> ZKey {
    return m_key;
}

auto Position::make_move(Move move) -> void {
    Piece mover          = piece_at(move.src());
    Piece captured_piece = move.special_type() == Move::kEnPassant
                           ? Piece{~m_side_to_move, PieceType::kPawn}
                           : piece_at(move.dst());
    bool  is_pawn_move   = mover.piece_type() == PieceType::kPawn;
    bool  is_capture     = !captured_piece.empty() && captured_piece.color() != m_side_to_move;

    if (m_en_passant_target_square != Square::kInvalid) {
        m_key ^= z_key_en_passant_file(m_en_passant_target_square.file());
    }

    m_en_passant_target_square = Square::kInvalid;

    switch (move.special_type()) {
    case Move::kNormal: {
        Piece target = piece_at(move.dst());
        if (!target.empty()) {
            delete_piece(move.dst());
        }

        if (mover.piece_type() == PieceType::kPawn) {
            if (std::abs(move.src().rank() - move.dst().rank()) == 2) {
                auto pawn_direction        = Direction::pawn_direction(side_to_move());
                m_en_passant_target_square = move.src() + pawn_direction;
                m_key ^= z_key_en_passant_file(m_en_passant_target_square.file());
            }
        }

        move_piece(move.src(), move.dst());
        break;
    }
    case Move::kPromotion: {
        Piece target = piece_at(move.dst());

        if (!target.empty()) {
            delete_piece(move.dst());
        }

        move_piece(move.src(), move.dst());
        mutate_piece(move.dst(), move.promotes_to());
        break;
    }
    case Move::kEnPassant: {
        Square en_passant_pawn{move.dst().file(), move.src().rank()};

        move_piece(move.src(), move.dst());
        delete_piece(en_passant_pawn);
        break;
    }
    case Move::kCastling: {
        move_piece(move.src(), move.dst());

        if (move.dst() == Square::kG1) {
            move_piece(Square::kH1, Square::kF1);
        } else if (move.dst() == Square::kC1) {
            move_piece(Square::kA1, Square::kD1);
        } else if (move.dst() == Square::kG8) {
            move_piece(Square::kH8, Square::kF8);
        } else if (move.dst() == Square::kC8) {
            move_piece(Square::kA8, Square::kD8);
        }
    }
    }

    m_key ^= z_key_castling_rights(m_castling_rights);

    if (piece_at(move.dst()).piece_type() == PieceType::kKing) {
        m_castling_rights.del_castling_rights(CastlingRights::from_color(m_side_to_move));
    }

    if (move.src() == Square::kH1 || move.dst() == Square::kH1) {
        m_castling_rights.del_castling_rights(CastlingRights::kWhiteKSide);
    }

    if (move.src() == Square::kA1 || move.dst() == Square::kA1) {
        m_castling_rights.del_castling_rights(CastlingRights::kWhiteQSide);
    }

    if (move.src() == Square::kH8 || move.dst() == Square::kH8) {
        m_castling_rights.del_castling_rights(CastlingRights::kBlackKSide);
    }

    if (move.src() == Square::kA8 || move.dst() == Square::kA8) {
        m_castling_rights.del_castling_rights(CastlingRights::kBlackQSide);
    }

    if (is_pawn_move || is_capture) {
        m_move_rule = 0;
    } else {
        ++m_move_rule;
    }

    m_key ^= z_key_castling_rights(m_castling_rights);

    m_side_to_move = ~m_side_to_move;
    m_key ^= z_key_side_to_move();
}

auto Position::add_piece(Square at, Piece to) -> void {
    PieceId piece_id = m_piece_list[to.color()].add_piece(at, to);
    Tile    tile{piece_id, to};

    m_key ^= z_key_piece_square(to, at);

    m_mail_box[at] = tile;
    m_bit_boards[to.color()][to.piece_type()].set_square(at);
}

auto Position::move_piece(Square src, Square dst) -> void {
    Tile tile              = tile_at(src);
    auto [piece_id, piece] = tile.unpack();

    m_key ^= z_key_piece_square(piece, src);
    m_key ^= z_key_piece_square(piece, dst);

    m_mail_box[dst] = tile;
    m_mail_box[src] = Tile{};
    m_piece_list[piece.color()].move_piece(piece_id, dst);
    m_bit_boards[piece.color()][piece.piece_type()].unset_square(src);
    m_bit_boards[piece.color()][piece.piece_type()].set_square(dst);
}

auto Position::delete_piece(Square at) -> void {
    auto [piece_id, piece] = tile_at(at).unpack();

    m_key ^= z_key_piece_square(piece, at);

    m_piece_list[piece.color()].delete_piece(piece_id);
    m_mail_box[at] = Tile{};
    m_bit_boards[piece.color()][piece.piece_type()].unset_square(at);
}

auto Position::mutate_piece(Square at, PieceType to) -> void {
    auto [piece_id, piece] = tile_at(at).unpack();
    Piece to_piece{piece.color(), to};

    m_key ^= z_key_piece_square(piece, at);
    m_key ^= z_key_piece_square(to_piece, at);

    m_piece_list[piece.color()].mutate_piece(piece_id, to);
    m_mail_box[at] = Tile{piece_id, to_piece};
    m_bit_boards[piece.color()][piece.piece_type()].unset_square(at);
    m_bit_boards[piece.color()][to].set_square(at);
}

namespace x88 {
auto to_x88(Square sq) -> i32 {
    const i32 sq_num = static_cast<i32>(sq);
    return sq_num + (sq_num & ~7);
}

auto from_x88(i32 sq_num) -> Square {
    return Square{static_cast<usize>((sq_num + (sq_num & 7)) >> 1)};
}

constexpr i32 n_orth     = 16;
constexpr i32 e_orth     = 1;
constexpr i32 s_orth     = -16;
constexpr i32 w_orth     = -1;
constexpr i32 ne_diag    = n_orth + e_orth;
constexpr i32 se_diag    = s_orth + e_orth;
constexpr i32 sw_diag    = s_orth + w_orth;
constexpr i32 nw_diag    = n_orth + w_orth;
constexpr i32 nne_horsie = n_orth + n_orth + e_orth;
constexpr i32 nee_horsie = n_orth + e_orth + e_orth;
constexpr i32 see_horsie = s_orth + e_orth + e_orth;
constexpr i32 sse_horsie = s_orth + s_orth + e_orth;
constexpr i32 ssw_horsie = s_orth + s_orth + w_orth;
constexpr i32 sww_horsie = s_orth + w_orth + w_orth;
constexpr i32 nww_horsie = n_orth + w_orth + w_orth;
constexpr i32 nnw_horsie = n_orth + n_orth + w_orth;

}  // namespace

auto Position::lazy_generate_attack_table() -> void {
    for (PieceId id : m_piece_list[Color::kWhite]) {
        generate_attacks_for(Color::kWhite, id);
    }

    for (PieceId id : m_piece_list[Color::kBlack]) {
        generate_attacks_for(Color::kBlack, id);
    }
}

auto Position::generate_attacks_for(Color color, PieceId piece_id) -> void {
    const auto [square, piece_type] = info_of(piece_id, color);

    if (piece_type == PieceType::kPawn && color == Color::kWhite) {
        generate_leaper(color, piece_id, square, x88::nw_diag);
        generate_leaper(color, piece_id, square, x88::ne_diag);
        return;
    }

    if (piece_type == PieceType::kPawn && color == Color::kBlack) {
        generate_leaper(color, piece_id, square, x88::sw_diag);
        generate_leaper(color, piece_id, square, x88::se_diag);
        return;
    }

    if (piece_type == PieceType::kKnight) {
        generate_leaper(color, piece_id, square, x88::nne_horsie);
        generate_leaper(color, piece_id, square, x88::nee_horsie);
        generate_leaper(color, piece_id, square, x88::see_horsie);
        generate_leaper(color, piece_id, square, x88::sse_horsie);
        generate_leaper(color, piece_id, square, x88::ssw_horsie);
        generate_leaper(color, piece_id, square, x88::sww_horsie);
        generate_leaper(color, piece_id, square, x88::nww_horsie);
        generate_leaper(color, piece_id, square, x88::nnw_horsie);
        return;
    }


    if (piece_type.is_orth()) {
        generate_slider(color, piece_id, square, x88::n_orth);
        generate_slider(color, piece_id, square, x88::e_orth);
        generate_slider(color, piece_id, square, x88::s_orth);
        generate_slider(color, piece_id, square, x88::w_orth);

        if (piece_type == PieceType::kBishop) {
            return;
        }
    }

    if (piece_type.is_diag()) {
        generate_slider(color, piece_id, square, x88::ne_diag);
        generate_slider(color, piece_id, square, x88::se_diag);
        generate_slider(color, piece_id, square, x88::sw_diag);
        generate_slider(color, piece_id, square, x88::nw_diag);

        if (piece_type == PieceType::kRook) {
            return;
        }
    }

    if (piece_type == PieceType::kKing) {
        generate_leaper(color, piece_id, square, x88::n_orth);
        generate_leaper(color, piece_id, square, x88::e_orth);
        generate_leaper(color, piece_id, square, x88::s_orth);
        generate_leaper(color, piece_id, square, x88::w_orth);
        generate_leaper(color, piece_id, square, x88::ne_diag);
        generate_leaper(color, piece_id, square, x88::se_diag);
        generate_leaper(color, piece_id, square, x88::sw_diag);
        generate_leaper(color, piece_id, square, x88::nw_diag);
        return;
    }
}

auto Position::generate_slider(Color color, PieceId piece_id, Square src, i32 direction) -> void {
    i32 x88_coordinate = x88::to_x88(src);

    while (true) {
        x88_coordinate += direction;

        if ((x88_coordinate & 0x88) != 0) {
            break;
        }

        Square coordinate = x88::from_x88(x88_coordinate);
        m_attack_table[color][coordinate].set_id(piece_id);

        if (piece_at(coordinate).is_some()) {
            break;
        }
    }
}

auto Position::generate_leaper(Color color, PieceId piece_id, Square src, i32 direction) -> void {
    const i32 x88_coordinate = x88::to_x88(src) + direction;

    if ((x88_coordinate & 0x88) != 0) {
        return;
    }

    Square coordinate = x88::from_x88(x88_coordinate);
    m_attack_table[color][coordinate].set_id(piece_id);
}


auto Position::calculate_pin_rays() -> void {
    for (PieceId rook_id : m_piece_list[~m_side_to_move].piece_type(PieceType::kRook)) {
        Square rook_square;
        std::tie(rook_square, std::ignore) = m_piece_list[~m_side_to_move].get_info(rook_id);

        if (king_square().orthogonal_to(rook_square)) {
            auto pin_ray = BitBoard::ray_exclusive(king_square(), rook_square);

            if ((pin_ray & pieces()).pop_count() == 1
                && (pin_ray & pieces(side_to_move())).pop_count() == 1) {
                m_pin_rays |= BitBoard::ray_inclusive(king_square(), rook_square);
            }
        }
    }

    for (PieceId bishop_id : m_piece_list[~m_side_to_move].piece_type(PieceType::kBishop)) {
        Square bishop_square;
        std::tie(bishop_square, std::ignore) = m_piece_list[~m_side_to_move].get_info(bishop_id);

        if (king_square().diagonal_to(bishop_square)) {
            auto pin_ray = BitBoard::ray_exclusive(king_square(), bishop_square);

            if ((pin_ray & pieces()).pop_count() == 1
                && (pin_ray & pieces(side_to_move())).pop_count() == 1) {
                m_pin_rays |= BitBoard::ray_inclusive(king_square(), bishop_square);
            }
        }
    }

    for (PieceId queen_id : m_piece_list[~m_side_to_move].piece_type(PieceType::kQueen)) {
        Square queen_square;
        std::tie(queen_square, std::ignore) = m_piece_list[~m_side_to_move].get_info(queen_id);

        if (king_square().orthogonal_to(queen_square) || king_square().diagonal_to(queen_square)) {
            auto pin_ray = BitBoard::ray_exclusive(king_square(), queen_square);

            if ((pin_ray & pieces()).pop_count() == 1
                && (pin_ray & pieces(side_to_move())).pop_count() == 1) {
                m_pin_rays |= BitBoard::ray_inclusive(king_square(), queen_square);
            }
        }
    }
}

}  // kerosene
