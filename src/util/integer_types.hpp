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

#include <chrono>
#include <cstdint>

namespace kerosene {

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using usize = std::size_t;
using isize = std::ptrdiff_t;
static_assert(sizeof(usize) == sizeof(isize));

using f32 = float;
using f64 = double;

namespace time {
using Clock        = std::chrono::steady_clock;
using TimePoint    = std::chrono::time_point<Clock>;
using Duration     = TimePoint::duration;
using FloatSeconds = std::chrono::duration<f64>;
using Milliseconds = std::chrono::duration<i64, std::milli>;

template<typename T>
constexpr T cast(const auto& x) {
    return std::chrono::duration_cast<T>(x);
}

constexpr auto nps(u64 nodes, const auto& elapsed) -> u64 {
    return static_cast<u64>(static_cast<f64>(nodes) / cast<FloatSeconds>(elapsed).count());
}
}  // namespace time

}
