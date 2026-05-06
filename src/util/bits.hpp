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
#include "template.hpp"


#include <type_traits>

namespace kerosene {

template<typename T>
    requires(std::is_integral_v<T>)
constexpr auto clear_lsb(T bits) -> T {
    return bits & (bits - 1);
}

template<typename T>
    requires(std::is_integral_v<T>)
constexpr auto lsb(T bits) -> T {
    return bits & -bits;
}

template<typename CastTo, typename Underlying = usize>
    requires(IntegralConstructible<CastTo>)
struct BitIterator {
    explicit BitIterator(Underlying raw) :
        m_raw{raw} {
    }

    auto operator++() -> BitIterator& {
        m_raw = clear_lsb(m_raw);
        return *this;
    }

    auto operator*() const -> CastTo {
        return CastTo{static_cast<FirstConstructibleIntegralT<CastTo>>(std::countr_zero(m_raw))};
    }

    friend constexpr auto operator==(const BitIterator&, const BitIterator&) -> bool = default;

private:
    Underlying m_raw{};
};

template<typename T>
    requires(std::is_integral_v<T>)
constexpr auto signum(T val) -> int {
    return val == 0 ? 0 : val > 0 ? 1 : -1;
}

}  // namespace kerosene
