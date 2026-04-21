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
#include <optional>
#include <string>
#include <string_view>

#include "common.hpp"

#include <utility>

namespace kerosene {

constexpr std::string_view kStartPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr std::string_view kKiwiPete =
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

class Color {
public:
    enum Underlying : u8 {
        kWhite = 0,
        kBlack,
    };

    static constexpr usize kNb = 2;

    /* implicit */ constexpr Color(Underlying raw) :
        m_raw(raw) {
    }

    /* implicit */ constexpr operator usize() const {
        return m_raw;
    }

    static auto parse(const std::string& color) -> Color {
        if (color == "w" || color == "W") {
            return kWhite;
        }

        return kBlack;
    }

    friend constexpr auto operator~(Color color) -> Color;

private:
    Underlying m_raw{};
};

constexpr auto operator~(Color color) -> Color {
    return static_cast<Color::Underlying>(std::to_underlying(color.m_raw) ^ 1);
}

class Direction {
public:
    enum Underlying {
        kNorth = 8,
        kSouth = -8,
    };

    /* implicit */ Direction(Underlying raw) :
        m_raw(raw) {
    }

    /* implicit */ constexpr operator usize() {
        return static_cast<usize>(m_raw);
    }

    static constexpr auto pawn_direction(Color color) -> Direction {
        return color == Color::kWhite ? kNorth : kSouth;
    }

private:
    Underlying m_raw{};
};

class Square {
public:
    // clang-format off
    enum Underlying : u8 {
        kA1 = 0, kB1, kC1, kD1, kE1, kF1, kG1, kH1,
        kA2, kB2, kC2, kD2, kE2, kF2, kG2, kH2,
        kA3, kB3, kC3, kD3, kE3, kF3, kG3, kH3,
        kA4, kB4, kC4, kD4, kE4, kF4, kG4, kH4,
        kA5, kB5, kC5, kD5, kE5, kF5, kG5, kH5,
        kA6, kB6, kC6, kD6, kE6, kF6, kG6, kH6,
        kA7, kB7, kC7, kD7, kE7, kF7, kG7, kH7,
        kA8, kB8, kC8, kD8, kE8, kF8, kG8, kH8,
        kInvalid = 0x80,
    };
    // clang-format on

    /* implicit */ constexpr Square(Underlying raw) :
        m_raw(raw) {
    }

    explicit constexpr Square(usize idx) :
        m_raw(static_cast<Underlying>(idx)) {
    }

    constexpr Square(u8 file, u8 rank) :
        m_raw(static_cast<Underlying>(rank * 8 + file)) {
    }

    static constexpr auto invalid() -> Square {
        return kInvalid;
    }

    [[nodiscard]] constexpr auto file() const -> u8 {
        return static_cast<u8>(m_raw) % 8;
    }

    [[nodiscard]] constexpr auto rank() const -> u8 {
        return static_cast<u8>(m_raw) / 8;
    }

    /* implicit */ constexpr operator usize() const {
        return m_raw;
    }

    static constexpr auto parse(std::string_view s) -> std::optional<Square> {
        if (s.size() != 2) {
            return std::nullopt;
        }

        char file_c = s[0];
        char rank_c = s[1];

        if (file_c < 'a' || file_c > 'h') {
            return std::nullopt;
        }

        if (rank_c < '1' || rank_c > '8') {
            return std::nullopt;
        }

        u8 file = static_cast<u8>(file_c - 'a');
        u8 rank = static_cast<u8>(rank_c - '1');

        return Square(file, rank);
    }

    friend constexpr auto operator+(Square square, Direction direction) -> Square;

private:
    Underlying m_raw = kInvalid;
};

auto constexpr operator+(Square square, Direction direction) -> Square {
    return Square{static_cast<usize>(square) + static_cast<usize>(direction)};
}

class PieceType {
public:
    enum Underlying : u8 {
        kEmpty = 0,
        kPawn  = 1,
        kKnight,
        kBishop,
        kRook,
        kQueen,
        kKing,
    };

    /* implicit */ PieceType(Underlying raw) :
        m_raw(raw) {
    }

    explicit PieceType(usize raw) :
        m_raw(static_cast<Underlying>(raw)) {
    }

    /* implicit */ constexpr operator usize() const {
        return m_raw;
    }

    static constexpr auto parse(char c) -> PieceType {
        switch (c) {

        case 'P':
        case 'p':
            return kPawn;
        case 'N':
        case 'n':
            return kKnight;
        case 'B':
        case 'b':
            return kBishop;
        case 'R':
        case 'r':
            return kRook;
        case 'Q':
        case 'q':
            return kQueen;
        case 'K':
        case 'k':
            return kKing;
        default:
            return kEmpty;
        }
    }

private:
    Underlying m_raw{kEmpty};
};

class Piece {
public:
    enum Underlying : u8 {
        kEmpty = 0,

        kWPawn = 1,
        kWKnight,
        kWBishop,
        kWRook,
        kWQueen,
        kWKing,

        kBPawn = 0b1000 | kWPawn,
        kBKnight,
        kBBishop,
        kBRook,
        kBQueen,
        kBKing,
    };

    Piece() = default;

    Piece(Color color, PieceType piece_type) {
        using U = std::underlying_type_t<Underlying>;

        U type_bits  = piece_type;
        U color_bits = color == Color::kWhite ? 0 : 0b1000;

        m_raw = static_cast<Underlying>(type_bits | color_bits);
    }

    /* implicit */ Piece(Underlying raw) :
        m_raw(raw) {
    }

    [[nodiscard]] constexpr static auto parse(char c) -> Piece {
        switch (c) {
        case 'P':
            return kWPawn;
        case 'N':
            return kWKnight;
        case 'B':
            return kWBishop;
        case 'R':
            return kWRook;
        case 'Q':
            return kWQueen;
        case 'K':
            return kWKing;

        case 'p':
            return kBPawn;
        case 'n':
            return kBKnight;
        case 'b':
            return kBBishop;
        case 'r':
            return kBRook;
        case 'q':
            return kBQueen;
        case 'k':
            return kBKing;

        default:
            return kEmpty;
        }
    }

    [[nodiscard]] constexpr auto color() const -> Color {
        return m_raw & 0b1000 ? Color::kBlack : Color::kWhite;
    }

    [[nodiscard]] constexpr auto piece_type() const -> PieceType {
        return static_cast<PieceType::Underlying>(m_raw & 0b111);
    }

    [[nodiscard]] auto to_string() const -> std::string {
        switch (m_raw) {
        case kWPawn:
            return "P";
        case kWKnight:
            return "N";
        case kWBishop:
            return "B";
        case kWRook:
            return "R";
        case kWQueen:
            return "Q";
        case kWKing:
            return "K";

        case kBPawn:
            return "p";
        case kBKnight:
            return "n";
        case kBBishop:
            return "b";
        case kBRook:
            return "r";
        case kBQueen:
            return "q";
        case kBKing:
            return "k";

        case kEmpty:
        default:
            return " ";
        }
    }

    [[nodiscard]] auto empty() const -> bool {
        return m_raw == kEmpty;
    }

private:
    Underlying m_raw{kEmpty};
};

}
