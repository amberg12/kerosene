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
#include <cstring>
#include "integer_types.hpp"

namespace kerosene {

// C++ has inplace_vector. This implements a subset of that.
template<typename T, usize kStackSize>
class inplace_vector {
public:
    using value_type             = T;
    using size_type              = usize;
    using difference_type        = isize;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr inplace_vector() noexcept = default;

    constexpr inplace_vector(const inplace_vector& other)
        requires(std::copyable<T>)
    {
        std::memcpy(data(), other.data(), other.size());
        m_size = other.size();
    }

    constexpr inplace_vector(inplace_vector&& other) noexcept(
      std::is_nothrow_move_constructible_v<T>)
        requires(std::movable<T>)
    {
        for (auto&& e : other) {
            emplace_back(std::move(e));
        }
        other.m_size = 0;
    }

    constexpr ~inplace_vector() noexcept {
        for (size_type i = 0; i < m_size; ++i) {
            destroy(i);
        }
    }

    // Element access
    constexpr auto at(size_type pos) -> reference {
        return *(data() + pos);
    }

    constexpr auto at(size_type pos) const -> const_reference {
        return *(data() + pos);
    }

    constexpr auto operator[](size_type pos) -> reference {
        return at(pos);
    }

    constexpr auto operator[](size_type pos) const -> const_reference {
        return at(pos);
    }

    constexpr auto front() -> reference {
        return at(0);
    }

    constexpr auto front() const -> const_reference {
        return at(0);
    }

    constexpr auto back() -> reference {
        return at(m_size - 1);
    }

    constexpr auto back() const -> const_reference {
        return at(m_size - 1);
    }

    constexpr auto data() noexcept -> T* {
        return reinterpret_cast<T*>(m_raw.data());
    }

    constexpr auto data() const noexcept -> const T* {
        return reinterpret_cast<const T*>(m_raw.data());
    }

    // Iterators
    constexpr auto begin() noexcept -> iterator {
        return data();
    }

    constexpr auto begin() const noexcept -> const_iterator {
        return data();
    }

    constexpr auto cbegin() const noexcept -> const_iterator {
        return data();
    }

    constexpr auto end() noexcept -> iterator {
        return data() + m_size;
    }

    constexpr auto end() const noexcept -> const_iterator {
        return data() + m_size;
    }

    constexpr auto cend() const noexcept -> const_iterator {
        return data() + m_size;
    }

    // Size and capacity
    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return m_size == 0;
    }

    [[nodiscard]] constexpr auto size() const noexcept -> size_type {
        return m_size;
    }

    static constexpr auto max_size() noexcept -> size_type {
        return kStackSize;
    }

    static constexpr auto capacity() noexcept -> size_type {
        return kStackSize;
    }

    // Modifiers
    template<typename... Args>
    constexpr auto emplace_back(Args&&... args) -> reference
        requires(std::constructible_from<T, Args...>)
    {
        std::construct_at(data() + m_size++, std::forward<Args>(args)...);
        return *std::addressof(back());
    }

private:
    auto destroy(size_type at) noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (std::is_trivial_v<T>) {
            return;
        }

        std::destroy_at(data() + at);
    }

    // We do a byte array rather than directly storing an array of T in order to prevent default
    // construction on unused elements.
    //
    // https://github.com/pkisensee/InplaceVector/tree/master
    //
    // We should have this as the first element so if we use alignas(usize) the underlying array is
    // also aligned.
    alignas(alignof(T)) std::array<u8, kStackSize * sizeof(T)> m_raw;

    size_type m_size{};
};

}  // namespace kerosene
