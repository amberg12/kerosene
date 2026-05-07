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
#include "evaluation_features.hpp"
#include "../types.hpp"

namespace kerosene::tuning {

class EvaluationTrace : public FeatureMap<i32> {
public:
    template <Color::Underlying kColor>
    auto increment_feature(EvalFeature eval_feat, i32 by) -> void {
        if (kColor == Color::kWhite) {
            feature(eval_feat) += by;
        } else {
            feature(eval_feat) -= by;
        }
    }

    template <Color::Underlying kColor>
    auto increment_feature(EvalFeature eval_feat) -> void {
        increment_feature<kColor>(eval_feat, 1);
    }
};

}  // namespace kerosene::tuning