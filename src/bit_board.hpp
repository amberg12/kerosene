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

#include "types.hpp"
#include "util/bits.hpp"
#include "util/integer_types.hpp"

namespace kerosene {

class BitBoard {
public:
    using Iterator = BitIterator<Square, u64>;

    BitBoard() = default;

    constexpr explicit BitBoard(Square square) :
        m_raw(1ull << static_cast<usize>(square)) {
    }

    constexpr explicit BitBoard(u64 raw) :
        m_raw(raw) {
    }

    constexpr static auto full() -> BitBoard {
        return BitBoard(~static_cast<u64>(0));
    }

    constexpr auto set_square(Square square) -> void {
        BitBoard set_bit_board{square};
        *this |= set_bit_board;
    }

    constexpr auto unset_square(Square square) -> void {
        BitBoard unset_bit_board{square};
        *this &= ~unset_bit_board;
    }

    [[nodiscard]] constexpr auto has_square(Square square) const -> bool {
        return !(*this & BitBoard{square}).empty();
    }

    [[nodiscard]] constexpr auto empty() const -> bool {
        return m_raw == 0;
    }

    [[nodiscard]] constexpr auto pop_count() const -> i32 {
        return std::popcount(m_raw);
    }

    [[nodiscard]] constexpr auto intersects(BitBoard rhs) const -> bool {
        return m_raw & rhs.m_raw;
    }

    [[nodiscard]] auto begin() const -> Iterator {
        return Iterator{m_raw};
    }

    [[nodiscard]] static auto end() -> Iterator {
        return Iterator{0};
    }

    friend constexpr auto operator^(BitBoard lhs, BitBoard rhs) -> BitBoard {
        return BitBoard{lhs.m_raw ^ rhs.m_raw};
    }

    friend constexpr auto operator^=(BitBoard& lhs, BitBoard rhs) -> BitBoard& {
        lhs = lhs ^ rhs;
        return lhs;
    }

    friend constexpr auto operator&(BitBoard lhs, BitBoard rhs) -> BitBoard {
        return BitBoard{lhs.m_raw & rhs.m_raw};
    }

    friend constexpr auto operator&=(BitBoard& lhs, BitBoard rhs) -> BitBoard& {
        lhs = BitBoard{lhs.m_raw & rhs.m_raw};
        return lhs;
    }

    friend constexpr auto operator|(BitBoard lhs, BitBoard rhs) -> BitBoard {
        return BitBoard{lhs.m_raw | rhs.m_raw};
    }

    friend constexpr auto operator|=(BitBoard& lhs, BitBoard rhs) -> BitBoard& {
        lhs = BitBoard{lhs.m_raw | rhs.m_raw};
        return lhs;
    }

    friend constexpr auto operator~(BitBoard bit_board) -> BitBoard {
        return BitBoard{~bit_board.m_raw};
    }

    static constexpr auto ray_exclusive(Square src, Square dst) {
        constexpr std::array<std::array<BitBoard, 64>, 64> kRays = [] {
            std::array<std::array<BitBoard, 64>, 64> out;

            for (Square src : kSquares) {
                for (Square dst : kSquares) {
                    if (src == dst) {
                        continue;
                    }

                    if (!src.diagonal_to(dst) && !src.orthogonal_to(dst)) {
                        continue;
                    }

                    i8 d_file = signum(dst.file() - src.file());
                    i8 d_rank = signum(dst.rank() - src.rank());

                    i8 file = src.file() + d_file;
                    i8 rank = src.rank() + d_rank;

                    BitBoard bb{};

                    while (file != dst.file() || rank != dst.rank()) {
                        bb |= BitBoard{Square(file, rank)};
                        file += d_file;
                        rank += d_rank;
                    }

                    out[src][dst] = bb;
                }
            }

            return out;
        }();

        return kRays[src][dst];
    }

    static constexpr auto ray_inclusive(Square src, Square dst) -> BitBoard {
        constexpr std::array<std::array<BitBoard, 64>, 64> kRays = [] {
            std::array<std::array<BitBoard, 64>, 64> out;

            for (Square src : kSquares) {
                for (Square dst : kSquares) {
                    if (!src.diagonal_to(dst) && !src.orthogonal_to(dst)) {
                        continue;
                    }

                    out[src][dst] = ray_exclusive(src, dst) | BitBoard{src} | BitBoard{dst};
                }
            }

            return out;
        }();

        return kRays[src][dst];
    }

private:
    u64 m_raw{};
};

}  // namespace kerosene
