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
#include "common.hpp"

namespace kerosene {

class Color {
public:
    enum Underlying : u8 {
        kWhite,
        kBlack,
    };

    static constexpr usize kNb = 2;

    /* implicit */ constexpr Color(Underlying raw) :
        m_raw(raw) {
    }

    /* implicit */ constexpr operator usize() const {
        return m_raw;
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

private:
    Underlying m_raw = kInvalid;
};

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
        kRRook,
        kRQueen,
        kRKing,
    };

    /* implicit */ Piece(Underlying raw) : m_raw(raw) {}

    [[nodiscard]] constexpr auto color() const -> Color {
        return m_raw & 0b1000 ? Color::kBlack : Color::kWhite;
    }

    [[nodiscard]] constexpr auto piece_type() const -> PieceType {
        return static_cast<PieceType::Underlying>(m_raw & 0b111);
    }

private:
    Underlying m_raw{};
};

}
