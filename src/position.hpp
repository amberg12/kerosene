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

#include <array>
#include <string_view>

#include "bit_board.hpp"
#include "move.hpp"
#include "types.hpp"
#include "util/bits.hpp"
#include "zobrist.hpp"

namespace kerosene {

class PieceId {
public:
    explicit constexpr PieceId() = default;

    explicit constexpr PieceId(u8 raw) :
        m_raw(raw) {
    }

    static constexpr auto king() -> PieceId {
        return PieceId{0};
    }

    explicit operator usize() const {
        return m_raw;
    }

    friend constexpr auto operator==(const PieceId&, const PieceId&) -> bool = default;

private:
    friend class PieceMask;

    [[nodiscard]] constexpr auto to_bit_position() const -> u16 {
        return 1 << m_raw;
    }

    u8 m_raw{0x10};
};

static_assert(sizeof(PieceId) == sizeof(u8));

class PieceMask {
public:
    using Iterator = BitIterator<PieceId, u16>;

    static constexpr usize kNb = 16;

    constexpr PieceMask() = default;

    explicit constexpr PieceMask(u16 raw) :
        m_raw(raw) {
    }

    constexpr auto set_id(PieceId id) -> void {
        m_raw |= id.to_bit_position();
    }

    constexpr auto unset_id(PieceId id) -> void {
        m_raw &= ~id.to_bit_position();
    }

    [[nodiscard]] constexpr auto has_id(PieceId id) const -> bool {
        return m_raw & 1 << static_cast<usize>(id);
    }

    [[nodiscard]] constexpr auto popcount() const -> i32 {
        return std::popcount(m_raw);
    }

    friend constexpr auto operator~(const PieceMask piece_mask) -> PieceMask {
        return PieceMask{static_cast<u16>(~piece_mask.m_raw)};
    }

    constexpr auto operator==(const PieceMask&) const -> bool = default;

    [[nodiscard]] auto begin() const -> Iterator {
        return Iterator{m_raw};
    }

    [[nodiscard]] static auto end() -> Iterator {
        return Iterator{0};
    }

private:
    u16 m_raw{0};
};

class Tile {
    static constexpr u8 kEmpty     = 0b0000'0000;
    static constexpr u8 kPieceMask = 0b0000'1111;
    static constexpr u8 kIdMask    = 0b1111'0000;

public:
    constexpr Tile() = default;

    constexpr Tile(PieceId id, Piece piece) {
        pack(id, piece);
    }

    [[nodiscard]] constexpr auto unpack() const -> std::pair<PieceId, Piece> {
        u8 id_bits    = (m_raw & kIdMask) >> 4;
        u8 piece_bits = (m_raw & kPieceMask);

        return {PieceId{id_bits}, Piece{static_cast<Piece::Underlying>(piece_bits)}};
    }

private:
    constexpr auto pack(PieceId id, Piece piece) -> void {
        u8 id_bits    = std::bit_cast<u8>(id);
        u8 piece_bits = std::bit_cast<u8>(piece);

        m_raw = id_bits << 4 | piece_bits;
    }

    u8 m_raw{};
};

class alignas(64) ByteBoard {
public:
    constexpr ByteBoard() = default;

    [[nodiscard]] constexpr auto operator[](Square square) -> Tile& {
        return m_byte_board[square];
    }

    [[nodiscard]] constexpr auto operator[](Square square) const -> const Tile& {
        return m_byte_board[square];
    }

private:
    std::array<Tile, 64> m_byte_board;
};

class alignas(64) word_board {
public:
    constexpr word_board() = default;

    [[nodiscard]] constexpr auto operator[](Square square) -> PieceMask& {
        return m_word_board[square];
    }

    [[nodiscard]] constexpr auto operator[](Square square) const -> const PieceMask& {
        return m_word_board[square];
    }

    [[nodiscard]] constexpr auto attacked_by(PieceId attacker) const -> BitBoard {
        BitBoard out{};

        for (Square target : kSquares) {
            if ((*this)[target].has_id(attacker)) {
                out.set_square(target);
            }
        }

        return out;
    }

    [[nodiscard]] constexpr auto mobility_of(PieceId piece_id) const -> i32 {
        i32 out{};

        for (PieceMask mask : m_word_board) {
            if (mask.has_id(piece_id)) {
                ++out;
            }
        }

        return out;
    }

    constexpr auto remove_attacker(PieceId piece_id) -> void {
        for (PieceMask& mask : m_word_board) {
            mask.unset_id(piece_id);
        }
    }

private:
    std::array<PieceMask, 64> m_word_board;
};

class PieceList {
public:
    constexpr PieceList() = default;

    auto add_piece(Square at, Piece to) -> PieceId;
    auto move_piece(PieceId piece_id, Square dst) -> void;
    auto delete_piece(PieceId piece_id) -> void;
    auto mutate_piece(PieceId piece_id, PieceType to) -> void;

    [[nodiscard]] auto get_info(PieceId piece_id) const -> std::pair<Square, PieceType> {
        usize idx = static_cast<usize>(piece_id);
        return {m_squares[idx], m_piece_types[idx]};
    }

    [[nodiscard]] auto begin() const -> PieceMask::Iterator {
        return m_piece_mask.begin();
    }

    [[nodiscard]] static auto end() -> PieceMask::Iterator {
        return PieceMask::end();
    }

    [[nodiscard]] auto piece_type(PieceType piece_type) const -> PieceMask {
        PieceMask out;
        for (PieceId id : m_piece_mask) {
            PieceType pt;
            std::tie(std::ignore, pt) = get_info(id);

            if (pt == piece_type) {
                out.set_id(id);
            }
        }

        return out;
    }

private:
    PieceMask                             m_piece_mask{};
    std::array<Square, PieceMask::kNb>    m_squares;
    std::array<PieceType, PieceMask::kNb> m_piece_types;
};

class CastlingRights {
public:
    enum Underlying : u8 {
        kWhiteKSide = 0b0001,
        kWhiteQSide = 0b0010,
        kWhite      = kWhiteKSide | kWhiteQSide,
        kBlackKSide = 0b0100,
        kBlackQSide = 0b1000,
        kBlack      = kBlackKSide | kBlackQSide,
    };

    /* implicit */ CastlingRights(Underlying raw) :
        m_raw(raw) {
    }

    /* implicit */ operator Underlying() const {
        return static_cast<Underlying>(m_raw);
    }

    static constexpr auto none() -> CastlingRights {
        return CastlingRights{static_cast<Underlying>(0)};
    }

    static constexpr auto from_color(Color color) -> CastlingRights {
        return color == Color::kWhite ? kWhite : kBlack;
    }

    constexpr auto add_casting_rights(CastlingRights castling_rights) -> void {
        m_raw |= castling_rights;
    }

    constexpr auto del_castling_rights(CastlingRights castling_rights)  -> void {
        m_raw &= ~castling_rights;
    }

    [[nodiscard]] constexpr auto has_castling_rights(CastlingRights castling_rights) const -> bool {
        return m_raw & castling_rights;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::string out;

        if (m_raw & kWhiteKSide) {
            out += "K";
        }

        if (m_raw & kWhiteQSide) {
            out += "Q";
        }

        if (m_raw & kBlackKSide) {
            out += "k";
        }

        if (m_raw & kBlackQSide) {
            out += "q";
        }

        return out.empty() ? "-" : out;
    }

private:
    u8 m_raw{};
};

class Position {
public:
    Position(const Position& parent, Move move);

    static auto parse(std::string_view fen) -> Position;

    [[nodiscard]] auto piece_at(Square at) const -> Piece;
    [[nodiscard]] auto tile_at(Square at) const -> Tile;
    [[nodiscard]] auto attackers_to(Square to) const -> ColorMap<PieceMask>;

    [[nodiscard]] auto info_of(PieceId id, Color color) const -> std::pair<Square, PieceType>;

    [[nodiscard]] auto to_string() const -> std::string;

    [[nodiscard]] auto castling_rights() const -> const CastlingRights&;
    [[nodiscard]] auto en_passant() const -> Square;
    [[nodiscard]] auto side_to_move() const -> Color;

    [[nodiscard]] auto piece_mask_for(Color side_to_move, PieceType piece_type) const -> PieceMask;
    [[nodiscard]] auto piece_count(Color side_to_move, PieceType piece_type) const -> i32;

    template<typename... Pts>
        requires((std::is_same_v<Pts, PieceType::Underlying> && ...) && sizeof...(Pts) > 0)
    [[nodiscard]] auto pieces(Color color, Pts... piece_types) const -> BitBoard {
        return (m_bit_boards[color][piece_types] | ...);
    }

    [[nodiscard]] auto pieces(Color color) const -> BitBoard;
    [[nodiscard]] auto pieces() const -> BitBoard;

    [[nodiscard]] auto phase() const -> i32;

    [[nodiscard]] auto attacked_by(Color color, PieceId id) const -> BitBoard;

    [[nodiscard]] auto mobility_of(Color color, PieceId piece_id) const -> i32;

    [[nodiscard]] auto king_square(Color side_to_move) const -> Square;
    [[nodiscard]] auto king_square() const -> Square;
    [[nodiscard]] auto checkers_nb() const -> i32;
    [[nodiscard]] auto check() const -> bool;
    [[nodiscard]] auto pin_rays() const -> BitBoard;

    [[nodiscard]] auto is_capture(Move move) const -> bool;

    [[nodiscard]] auto hash() const -> ZKey;
private:
    Position() = default;

    auto make_move(Move move) -> void;

    auto add_piece(Square at, Piece to) -> void;
    auto move_piece(Square src, Square dst) -> void;
    auto delete_piece(Square at) -> void;
    auto mutate_piece(Square at, PieceType to) -> void;

    auto lazy_generate_attack_table() -> void;

    auto generate_attacks_for(Color color, PieceId piece_id) -> void;
    auto generate_slider(Color color, PieceId piece_id, Square src, i32 direction) -> void;
    auto generate_leaper(Color color, PieceId piece_id, Square src, i32 direction) -> void;

    auto remove_attacker(Color color, PieceId piece_id) -> void;
    auto update_sliders_to(Square src) -> void;
    auto update_slider(Color color, PieceId piece_id, Square to) -> void;

    auto calculate_pin_rays() -> void;

    ColorMap<word_board>              m_attack_table{};
    ByteBoard                        m_mail_box{};
    ColorMap<PieceList>              m_piece_list{};
    ColorMap<PieceTypeMap<BitBoard>> m_bit_boards{};

    BitBoard m_pin_rays{};

    ZKey m_key{};

    Color          m_side_to_move{Color::kWhite};
    u8             m_move_rule{0};
    CastlingRights m_castling_rights{CastlingRights::none()};
    Square         m_en_passant_target_square{Square::kInvalid};
};

}  // kerosene
