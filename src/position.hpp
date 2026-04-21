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
#include "move.hpp"


#include <array>
#include <string_view>

#include "types.hpp"

namespace kerosene {

class alignas(64) Mailbox {
public:
    Mailbox() = default;

    constexpr auto operator[](Square at) -> Piece& {
        return m_mailbox[at];
    }

    constexpr auto operator[](Square at) const -> const Piece& {
        return m_mailbox[at];
    }

private:
    std::array<Piece, 64> m_mailbox;
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

    /* implicit */ CastlingRights(Underlying raw) : m_raw(raw) {}

    /* implicit */ operator Underlying() const {
        return static_cast<Underlying>(m_raw);
    }

    static constexpr auto none() -> CastlingRights {
        return CastlingRights{static_cast<Underlying>(0)};
    }

    static constexpr auto from_color(Color color) -> CastlingRights {
        return color == Color::kWhite ? kWhite : kBlack;
    }

    constexpr void add_casting_rights(CastlingRights castling_rights) {
        m_raw |= castling_rights;
    }

    constexpr void del_castling_rights(CastlingRights castling_rights) {
        m_raw &= ~castling_rights;
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

    [[nodiscard]] auto tile_at(Square at) const -> Piece;

    [[nodiscard]] auto to_string() const -> std::string;

    [[nodiscard]] auto castling_rights() const -> const CastlingRights&;
    [[nodiscard]] auto en_passant() const -> Square;

private:
    Position() = default;

    auto make_move(Move move) -> void;

    auto add_piece(Square at, Piece to) -> void;
    auto move_piece(Square src, Square dst) -> void;
    auto delete_piece(Square at) -> void;
    auto mutate_piece(Square at, PieceType to) -> void;

    Mailbox m_mailbox{};

    Color          m_side_to_move{Color::kWhite};
    u8             m_move_rule{0};
    CastlingRights m_castling_rights{CastlingRights::none()};
    Square         m_en_passant_target_square{Square::kInvalid};
};

}  // kerosene
