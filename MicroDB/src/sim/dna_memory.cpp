#include "dna_memory.h"

#include <algorithm>

void DNAMemory::add(const SimParams &params, const Genome &genome, float fitness, const EvoParams &evo, int capacity_override,
                    const std::string &style) {
    entries.push_back({genome, fitness, 0, style});
    std::sort(entries.begin(), entries.end(), [](const DNAEntry &a, const DNAEntry &b) {
        return a.fitness > b.fitness;
    });
    int capacity = (capacity_override > 0) ? capacity_override : params.dna_capacity;
    if (static_cast<int>(entries.size()) > capacity) {
        entries.resize(capacity);
    }
}

Genome DNAMemory::sample(Rng &rng, const SimParams &params, const EvoParams &evo) const {
    if (entries.empty()) {
        Genome g;
        g.sense_gain = rng.uniform(0.6f, 1.4f);
        g.pheromone_gain = rng.uniform(0.6f, 1.4f);
        g.exploration_bias = rng.uniform(0.2f, 0.8f);
        g.edge_seek = rng.uniform(-0.3f, 0.3f);
        g.soften_bias = rng.uniform(0.2f, 0.8f);
        g.mycel_drive = rng.uniform(0.2f, 1.2f);
        g.blur_pref = rng.uniform(0.2f, 0.8f);
        g.sharpen_pref = rng.uniform(0.2f, 0.8f);
        g.color_shift_r = rng.uniform(-0.2f, 0.2f);
        g.color_shift_g = rng.uniform(-0.2f, 0.2f);
        g.color_shift_b = rng.uniform(-0.2f, 0.2f);
        g.contrast_pulse = rng.uniform(0.0f, 0.6f);
        return g;
    }

    auto clamp01 = [](float v) {
        return std::min(1.0f, std::max(0.0f, v));
    };

    auto clamp_range = [](float v, float lo, float hi) {
        return std::min(hi, std::max(lo, v));
    };

    auto weighted_pick = [&](const std::vector<DNAEntry> &pool) -> Genome {
        float total = 0.0f;
        for (const auto &entry : pool) {
            total += entry.fitness * params.dna_survival_bias + 0.01f;
        }
        float pick = rng.uniform(0.0f, total);
        for (const auto &entry : pool) {
            float w = entry.fitness * params.dna_survival_bias + 0.01f;
            if (pick <= w) {
                return entry.genome;
            }
            pick -= w;
        }
        return pool.front().genome;
    };

    Genome g;
    if (evo.enabled) {
        int elite_count = std::max(1, static_cast<int>(entries.size() * evo.elite_frac));
        bool from_elite = (rng.uniform(0.0f, 1.0f) < evo.elite_frac);
        if (from_elite && elite_count > 0) {
            std::vector<DNAEntry> elite(entries.begin(), entries.begin() + elite_count);
            g = weighted_pick(elite);
        } else {
            g = weighted_pick(entries);
        }
        g.sense_gain *= rng.uniform(1.0f - evo.mutation_sigma, 1.0f + evo.mutation_sigma);
        g.pheromone_gain *= rng.uniform(1.0f - evo.mutation_sigma, 1.0f + evo.mutation_sigma);
        g.exploration_bias = clamp01(g.exploration_bias + rng.uniform(-evo.exploration_delta, evo.exploration_delta));
        g.edge_seek = clamp_range(g.edge_seek + rng.uniform(-evo.exploration_delta, evo.exploration_delta), -1.0f, 1.0f);
        g.soften_bias = clamp01(g.soften_bias + rng.uniform(-evo.exploration_delta, evo.exploration_delta));
        g.mycel_drive *= rng.uniform(1.0f - evo.mutation_sigma, 1.0f + evo.mutation_sigma);
        g.blur_pref = clamp01(g.blur_pref + rng.uniform(-evo.exploration_delta, evo.exploration_delta));
        g.sharpen_pref = clamp01(g.sharpen_pref + rng.uniform(-evo.exploration_delta, evo.exploration_delta));
        g.color_shift_r = clamp_range(g.color_shift_r + rng.uniform(-evo.exploration_delta, evo.exploration_delta) * 0.5f, -0.5f, 0.5f);
        g.color_shift_g = clamp_range(g.color_shift_g + rng.uniform(-evo.exploration_delta, evo.exploration_delta) * 0.5f, -0.5f, 0.5f);
        g.color_shift_b = clamp_range(g.color_shift_b + rng.uniform(-evo.exploration_delta, evo.exploration_delta) * 0.5f, -0.5f, 0.5f);
        g.contrast_pulse = clamp01(g.contrast_pulse + rng.uniform(-evo.exploration_delta, evo.exploration_delta));
    } else {
        g = weighted_pick(entries);
        g.sense_gain *= rng.uniform(0.9f, 1.1f);
        g.pheromone_gain *= rng.uniform(0.9f, 1.1f);
        g.exploration_bias = clamp01(g.exploration_bias + rng.uniform(-0.05f, 0.05f));
        g.edge_seek = clamp_range(g.edge_seek + rng.uniform(-0.05f, 0.05f), -1.0f, 1.0f);
        g.soften_bias = clamp01(g.soften_bias + rng.uniform(-0.05f, 0.05f));
        g.mycel_drive *= rng.uniform(0.95f, 1.05f);
        g.blur_pref = clamp01(g.blur_pref + rng.uniform(-0.05f, 0.05f));
        g.sharpen_pref = clamp01(g.sharpen_pref + rng.uniform(-0.05f, 0.05f));
        g.color_shift_r = clamp_range(g.color_shift_r + rng.uniform(-0.02f, 0.02f), -0.5f, 0.5f);
        g.color_shift_g = clamp_range(g.color_shift_g + rng.uniform(-0.02f, 0.02f), -0.5f, 0.5f);
        g.color_shift_b = clamp_range(g.color_shift_b + rng.uniform(-0.02f, 0.02f), -0.5f, 0.5f);
        g.contrast_pulse = clamp01(g.contrast_pulse + rng.uniform(-0.03f, 0.03f));
    }

    g.sense_gain = clamp_range(g.sense_gain, 0.2f, 3.0f);
    g.pheromone_gain = clamp_range(g.pheromone_gain, 0.2f, 3.0f);
    g.exploration_bias = clamp01(g.exploration_bias);
    g.edge_seek = clamp_range(g.edge_seek, -1.0f, 1.0f);
    g.soften_bias = clamp01(g.soften_bias);
    g.mycel_drive = clamp_range(g.mycel_drive, 0.0f, 2.0f);
    g.blur_pref = clamp01(g.blur_pref);
    g.sharpen_pref = clamp01(g.sharpen_pref);
    g.color_shift_r = clamp_range(g.color_shift_r, -0.5f, 0.5f);
    g.color_shift_g = clamp_range(g.color_shift_g, -0.5f, 0.5f);
    g.color_shift_b = clamp_range(g.color_shift_b, -0.5f, 0.5f);
    g.contrast_pulse = clamp01(g.contrast_pulse);
    return g;
}

void DNAMemory::decay(const EvoParams &evo) {
    float decay = evo.enabled ? evo.age_decay : 0.995f;
    for (auto &entry : entries) {
        entry.age += 1;
        entry.fitness *= decay;
    }
}
