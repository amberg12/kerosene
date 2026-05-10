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

#include "../util/integer_types.hpp"

namespace kerosene::tuning {

constexpr usize kPsqtCount = 64;

enum class EvalFeature {
    kPawnMaterial,
    kKnightMaterial,
    kBishopMaterial,
    kRookMaterial,
    kQueenMaterial,
    kPawnPsqt,
    kKnightPsqt     = kPawnPsqt + kPsqtCount,
    kBishopPsqt     = kKnightPsqt + kPsqtCount,
    kRookPsqt       = kBishopPsqt + kPsqtCount,
    kQueenPsqt      = kRookPsqt + kPsqtCount,
    kKingPsqt       = kQueenPsqt + kPsqtCount,
    kMobilityKnight = kKingPsqt + kPsqtCount,
    kMobilityBishop = kMobilityKnight + 9,
    kMobilityRook   = kMobilityBishop + 14,
    kMobilityQueen  = kMobilityRook + 15,
    kNb             = kMobilityQueen + 28,
};

constexpr usize kFeatureCount = static_cast<usize>(EvalFeature::kNb);

template<typename V>
class FeatureMap {
public:
    using Array = std::array<V, kFeatureCount>;

    FeatureMap() = default;

    auto feature(EvalFeature eval_feat) -> V& {
        V& feat = m_underlying[static_cast<usize>(eval_feat)];
        return feat;
    }

    auto feature(EvalFeature eval_feat) const -> const V& {
        const V& feat = m_underlying[static_cast<usize>(eval_feat)];
        return feat;
    }

    auto feature(EvalFeature eval_feat, Square square) -> V& {
        usize idx = static_cast<usize>(eval_feat) + square;
        return feature(idx);
    }

    auto feature(EvalFeature eval_feat, Square square) const -> const V& {
        usize idx = static_cast<usize>(eval_feat) + square;
        return feature(idx);
    }

    auto feature(EvalFeature eval_feat, i32 offset) -> V& {
        usize idx = static_cast<usize>(eval_feat) + offset;
        return feature(idx);
    }

    auto feature(EvalFeature eval_feat, i32 offset) const -> const V& {
        usize idx = static_cast<usize>(eval_feat) + offset;
        return feature(idx);
    }

    auto feature(usize raw_feature_idx) -> V& {
        return m_underlying[raw_feature_idx];
    }

    auto feature(usize raw_feature_idx) const -> const V& {
        return m_underlying[raw_feature_idx];
    }

    auto as_array() const -> const Array& {
        return m_underlying;
    }

    auto as_array() -> Array& {
        return m_underlying;
    }

private:
    Array m_underlying{};
};

}  // namespace kerosene::tuning
