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

#include "sgdm.hpp"
#include "../evaluation.hpp"
#include "../evaluation_constants.hpp"
#include "../position.hpp"
#include "evaluation_features.hpp"
#include "evaluation_trace.hpp"

#include <cmath>
#include <print>
#include <random>

namespace kerosene::tuning {
constexpr i32 kResultScale = 650;
constexpr f64 kMu          = 0.9;
constexpr i32 kPatience    = 10;

auto scale_score(f64 score) -> Score {
    return score * kResultScale;
}

auto descale_score(Score score) -> f64 {
    return static_cast<f64>(score) / kResultScale;
}

#define KEROSENE_PRINT_FEATURE(table, feat)                                                 \
    std::println("constexpr ScorePair " #feat " = ScorePair{{{}, {}}};\n",                  \
                 scale_score(table.feature(feat).mg), scale_score(table.feature(feat).eg));

#define KEROSENE_PRINT_PSQT(table, feat)                                           \
    std::println("// clang-format off");                                           \
    std::println("constexpr std::array<ScorePair, Square::kNb> " #feat " = {{{{"); \
    for (i32 rank = 0; rank < 8; ++rank) {                                         \
        std::print("    ");                                                        \
        for (i32 file = 0; file < 8; ++file) {                                     \
            i32        sq = rank * 8 + file;                                       \
            const auto v  = table.feature(static_cast<usize>(feat) + sq);          \
            std::print("{{{}, {}}}, ", scale_score(v.mg), scale_score(v.eg));      \
        }                                                                          \
        std::println();                                                            \
    }                                                                              \
    std::println("}}}};");                                                         \
    std::println("// clang-format on\n");

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

class SgdmOut : public FeatureMap<Phase<f64>> {
public:
    auto load_feature(EvalFeature feat, ScorePair score_pair) -> void {
        feature(feat) = Phase{descale_score(score_pair.mg), descale_score(score_pair.eg)};
    }

    auto load_feature(EvalFeature feat, std::array<ScorePair, 64> score_pairs) -> void {
        for (usize i = 0; i < 64; ++i) {
            ScorePair score_pair = score_pairs[i];

            feature(static_cast<usize>(feat) + i) =
              Phase{descale_score(score_pair.mg), descale_score(score_pair.eg)};
        }
    }
};

// Taken from the video on sarah, https://www.youtube.com/watch?v=QzS8HQ3dVWA
auto sgdm(Dataset& dataset, i32 epochs, f64 lr, i32 batch_size, f64 lambda) -> void {
    std::mt19937 rng{std::random_device{}()};

    SgdmOut W{};
    W.load_feature(EvalFeature::kPawnMaterial, evaluation_constants::kPawnMaterial);
    W.load_feature(EvalFeature::kKnightMaterial, evaluation_constants::kKnightMaterial);
    W.load_feature(EvalFeature::kBishopMaterial, evaluation_constants::kBishopMaterial);
    W.load_feature(EvalFeature::kRookMaterial, evaluation_constants::kRookMaterial);
    W.load_feature(EvalFeature::kQueenMaterial, evaluation_constants::kQueenMaterial);
    W.load_feature(EvalFeature::kPawnPsqt, evaluation_constants::kPawnPsqt);
    W.load_feature(EvalFeature::kKnightPsqt, evaluation_constants::kKnightPsqt);
    W.load_feature(EvalFeature::kBishopPsqt, evaluation_constants::kBishopPsqt);
    W.load_feature(EvalFeature::kRookPsqt, evaluation_constants::kRookPsqt);
    W.load_feature(EvalFeature::kQueenPsqt, evaluation_constants::kQueenPsqt);
    W.load_feature(EvalFeature::kKingPsqt, evaluation_constants::kKingPsqt);

    FeatureMap<Phase<f64>> gradient_phase{};
    FeatureMap<Phase<f64>> W_phase_prev = W;
    FeatureMap<Phase<f64>> M_phase{};

    f64 bias = 0.0;
    i32 M    = dataset.size();
    i32 T    = 10;

    std::vector<usize> indices;

    for (usize i = 0; i < M; ++i) {
        indices.push_back(i);
    }

    f64 lr_min = 0.001;
    f64 lr_max = 0.01;

    i32 tc = 0;

    i32 epochs_without_improvement = 0;

    for (i32 epoch = 0; epoch < epochs; ++epoch) {
        time::TimePoint epoch_start = time::Clock::now();

        if (tc == T) {
            T *= 2;
            tc = 0;
            lr_min *= 0.95;
            lr_max *= 0.95;

            M_phase = FeatureMap<Phase<f64>>{};
        }

        f64 local_lr = lr_min + 0.5 * (lr_max - lr_min) * (1 + std::cos(std::numbers::pi * tc / T));
        tc++;

        std::shuffle(indices.begin(), indices.end(), rng);

        gradient_phase    = FeatureMap<Phase<f64>>{};
        f64 gradient_bias = 0.0;

        i32 batch_pos = 0;

        for (i32 k = 0; k < M; ++k) {
            const DatasetEntry& entry = dataset.at(indices[k]);

            const EvaluationTrace& X_i = entry.X_i;

            Phase<f64> dot{};
            f64        phase = static_cast<f64>(entry.phase) / 24.0;

            for (usize i = 0; i < kFeatureCount; ++i) {
                dot += W.feature(i) * X_i.feature(i);
            }

            f64 eval = phase * dot.mg + (1.0 - phase) * dot.eg;
            f64 p    = 1.0 / (1.0 + std::exp(-eval));
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
                batch_pos      = 0;
            }
        }

        f64 delta_sum = 0.0;
        f64 magnitude = 0.0;

        for (i32 i = 0; i < kFeatureCount; ++i) {
            Phase<f64> d{
              W.feature(i).mg - W_phase_prev.feature(i).mg,
              W.feature(i).eg - W_phase_prev.feature(i).eg,
            };

            delta_sum += std::abs(d.mg) + std::abs(d.eg);
            magnitude += W.feature(i).mg + W.feature(i).eg;
        }

        f64 delta_mean     = delta_sum / kFeatureCount;
        f64 magnitude_mean = magnitude / kFeatureCount;

        auto time_for_epoch =
          std::chrono::duration_cast<std::chrono::milliseconds>(time::Clock::now() - epoch_start)
            .count();
        auto kpos_per_second = (M / static_cast<f64>(time_for_epoch)) / 1000.0 * 1000.0;

        std::println(
          "Epoch {} | Time {}ms | {:1f}kpos/s | Delta sum {:6f} | Delta mean {:6f} | Magnitude {:6f} | Mean {:6f}",
          epoch, time_for_epoch, kpos_per_second, delta_sum, delta_mean, magnitude, magnitude_mean);

        if (delta_mean < 1e-4) {
            ++epochs_without_improvement;
        } else {
            epochs_without_improvement = 0;
        }

        if (epochs_without_improvement > kPatience) {
            break;
        }

        W_phase_prev = W;
    }

    {
        using enum EvalFeature;

        KEROSENE_PRINT_FEATURE(W, kPawnMaterial);
        KEROSENE_PRINT_FEATURE(W, kKnightMaterial);
        KEROSENE_PRINT_FEATURE(W, kBishopMaterial);
        KEROSENE_PRINT_FEATURE(W, kRookMaterial);
        KEROSENE_PRINT_FEATURE(W, kQueenMaterial);
        KEROSENE_PRINT_PSQT(W, kPawnPsqt);
        KEROSENE_PRINT_PSQT(W, kKnightPsqt);
        KEROSENE_PRINT_PSQT(W, kBishopPsqt);
        KEROSENE_PRINT_PSQT(W, kRookPsqt);
        KEROSENE_PRINT_PSQT(W, kQueenPsqt);
        KEROSENE_PRINT_PSQT(W, kKingPsqt);
    }
}
}  // namespace kerosene::tuning
