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

#include "../common.hpp"
#include <concepts>
#include <type_traits>

namespace kerosene {

template<typename T, typename... Candidates>
struct FirstConstructibleIntegral;

template<typename T, typename First, typename... Rest>
struct FirstConstructibleIntegral<T, First, Rest...> {
    using Type = std::conditional_t<std::constructible_from<T, First>,
                                    First,
                                    typename FirstConstructibleIntegral<T, Rest...>::Type>;
};

template<typename T>
struct FirstConstructibleIntegral<T> {
    using Type = void;
};

template<typename CastTo>
using FirstConstructibleIntegralT =
  FirstConstructibleIntegral<CastTo, u8, i8, u16, i16, u32, i32, u64, i64>::Type;
template<typename T>
concept IntegralConstructible = std::constructible_from<T, i8> || std::constructible_from<T, u8>
                             || std::constructible_from<T, i16> || std::constructible_from<T, u16>
                             || std::constructible_from<T, i32> || std::constructible_from<T, u32>
                             || std::constructible_from<T, i64> || std::constructible_from<T, u64>;

}
