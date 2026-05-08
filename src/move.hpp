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
#include <string>
#include "types.hpp"
#include "util/integer_types.hpp"

namespace kerosene {
class Position;

class Move {
public:
    enum MoveType {
        kNormal,
        kEnPassant,
        kCastling,
        kPromotion,
    };

    constexpr Move() = default;

    constexpr Move(Square src, Square dst) {
        u16 src_bits = src;
        u16 dst_bits = dst << 6;

        m_raw = src_bits | dst_bits;
    }

    static constexpr auto create_en_passant(Square src, Square dst) -> Move {
        Move out{src, dst};
        out.m_raw |= kEnPassantMask;
        return out;
    }

    static constexpr auto create_castling(Square src, Square dst) -> Move {
        Move out{src, dst};
        out.m_raw |= kCastlingMask;
        return out;
    }

    static constexpr auto create_promotion(Square src, Square dst, PieceType to) -> Move {
        Move out{src, dst};
        out.m_raw |= kPromotionMask;
        u16 promotion_index = static_cast<usize>(to) - static_cast<usize>(PieceType::kKnight);
        out.m_raw |= promotion_index << 12;

        return out;
    }

    [[nodiscard]] constexpr auto src() const -> Square {
        return Square{static_cast<usize>(m_raw & kSrcMask)};
    }

    [[nodiscard]] constexpr auto dst() const -> Square {
        return Square{static_cast<usize>((m_raw & kDstMask) >> 6)};
    }

    [[nodiscard]] constexpr auto special_type() const -> MoveType {
        switch (m_raw & kSpecialMask) {
        case 0:
            return kNormal;
        case kEnPassantMask:
            return kEnPassant;
        case kCastlingMask:
            return kCastling;
        default:
            return kPromotion;
        }
    }

    [[nodiscard]] constexpr auto promotes_to() const -> PieceType {
        u16 promote_to_idx = (m_raw & kPromoteToMask) >> 12;

        return PieceType{static_cast<usize>(PieceType::kKnight) + promote_to_idx};
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::string out;
        out += src().to_string();
        out += dst().to_string();

        if (special_type() == kPromotion) {
            switch (promotes_to()) {
            case PieceType::kKnight:
                out += "n";
                break;
            case PieceType::kBishop:
                out += "b";
                break;
            case PieceType::kRook:
                out += "r";
                break;
            case PieceType::kQueen:
                out += "q";
                break;
            default:;
            }
        }

        return out;
    }

    [[nodiscard]] static auto parse(const std::string& move, const Position& context) -> Move;

    [[nodiscard]] constexpr operator bool() const {
        return m_raw != 0;
    }

private:
    enum Flags : u16 {
        kSrcMask       = 0b0000'0000'0011'1111,
        kDstMask       = 0b0000'1111'1100'0000,
        kSpecialMask   = 0b0111'0000'0000'0000,
        kEnPassantMask = 0b0001'0000'0000'0000,
        kCastlingMask  = 0b0010'0000'0000'0000,
        kPromotionMask = 0b0100'0000'0000'0000,
        kPromoteToMask = 0b0011'0000'0000'0000,
    };

    u16 m_raw{};
};

constexpr Move kNullMove = Move{};

}
