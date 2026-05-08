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

#include "../evaluation.hpp"
#include "../position.hpp"
#include "evaluation_features.hpp"
#include "evaluation_trace.hpp"
#include "sgdm.hpp"
#include <cmath>
#include <print>
#include <random>

namespace kerosene::tuning {
constexpr i32 kResultScale = 650;
constexpr f64 kMu          = 0.9;

auto scale_score(f64 score) -> Score {
    constexpr f64 kClamp = 6.0;
    score = std::clamp(score, -kClamp, kClamp);
    return score * kResultScale;
}

#define KEROSENE_PRINT_FEATURE(table, feat) \
    std::println("constexpr ScorePair " #feat \
    " = ScorePair{{{}, {}}};", \
    scale_score(table.feature(feat).mg), scale_score(table.feature(feat).eg));

template<typename T>
struct Phase {
    T mg;
    T eg;

    friend auto operator+=(Phase& lhs, Phase rhs) -> Phase& {
        lhs.mg += rhs.mg;
        lhs.eg += rhs.eg;
        return lhs;
    }

    friend auto operator*(Phase lhs, i32 rhs) -> Phase {
        Phase out{};
        out.mg = lhs.mg * rhs;
        out.eg = lhs.eg * rhs;
        return out;
    }
};

// Taken from the video on sarah, https://www.youtube.com/watch?v=QzS8HQ3dVWA
auto sgdm(Dataset& dataset, i32 epochs, f64 lr, i32 batch_size, f64 lambda) -> void {
    std::mt19937 rng{std::random_device{}()};

    FeatureMap<Phase<f64>> W{};
    FeatureMap<Phase<f64>> gradient_phase{};
    FeatureMap<Phase<f64>> W_phase_prev{};
    FeatureMap<Phase<f64>> M_phase{};

    f64 bias = 0.0;
    i32 M    = dataset.size();
    i32 T    = 10;

    f64 lr_min = 0.001;
    f64 lr_max = 0.01;

    i32 tc = 0;

    for (i32 epoch = 0; epoch < epochs; ++epoch) {
        if (tc == T) {
            T *= 2;
            tc = 0;
            lr_min *= 0.95;
            lr_max *= 0.95;

            M_phase = FeatureMap<Phase<f64>>{};
        }

        f64 local_lr = lr_min + 0.5 * (lr_max - lr_min) * (1 + std::cos(std::numbers::pi * tc / T));
        tc++;

        std::shuffle(dataset.begin(), dataset.end(), rng);

        gradient_phase    = FeatureMap<Phase<f64>>{};
        f64 gradient_bias = 0.0;

        i32 batch_pos = 0;

        for (i32 k = 0; k < M; ++k) {
            const DatasetEntry& entry = dataset.at(k);

            auto            position = Position::parse(entry.fen);
            EvaluationTrace X_i{};

            (void)evaluate<true>(position, &X_i);

            Phase<f64> dot{};
            f64        phase = static_cast<f64>(position.phase()) / 24.0;

            for (usize i = 0; i < kFeatureCount; ++i) {
                dot += W.feature(i) * X_i.feature(i);
            }

            f64 eval = phase * dot.mg + (1.0 - phase) * dot.eg;
            f64 p = 1.0 / (1.0 + std::exp(-eval));
            f64 diff = p - parse(entry.result);

            for (i32 i = 0; i < kFeatureCount; ++i) {
                gradient_phase.feature(i).mg += diff * phase * X_i.feature(i);
                gradient_phase.feature(i).eg += diff * (1.0 - phase) * X_i.feature(i);
            }

            ++batch_pos;

            if (batch_pos == batch_size || k == M - 1) {
                for (i32 i = 0; i < kFeatureCount; ++i) {
                    Phase<f64> g{
                        gradient_phase.feature(i).mg / batch_pos + lambda * W.feature(i).mg,
                        gradient_phase.feature(i).eg / batch_pos + lambda * W.feature(i).eg,
                    };

                    M_phase.feature(i).mg = kMu * M_phase.feature(i).mg + g.mg;
                    M_phase.feature(i).eg = kMu * M_phase.feature(i).eg + g.eg;

                    W.feature(i).mg -= M_phase.feature(i).mg * local_lr;
                    W.feature(i).eg -= M_phase.feature(i).eg * local_lr;
                }

                gradient_phase = FeatureMap<Phase<f64>>{};
                batch_pos = 0;
            }
        }
    }

    {
        using enum EvalFeature;

        KEROSENE_PRINT_FEATURE(W, kPawnMaterial);
        KEROSENE_PRINT_FEATURE(W, kKnightMaterial);
        KEROSENE_PRINT_FEATURE(W, kBishopMaterial);
        KEROSENE_PRINT_FEATURE(W, kRookMaterial);
        KEROSENE_PRINT_FEATURE(W, kQueenMaterial);
    }
}
}  // namespace kerosene::tuning
