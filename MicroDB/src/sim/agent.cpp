#include "agent.h"

#include <cmath>

namespace {
float wrap_angle(float a) {
    const float two_pi = 6.283185307f;
    while (a < 0.0f) a += two_pi;
    while (a >= two_pi) a -= two_pi;
    return a;
}

float clamp_pitch(float p) {
    const float limit = 1.55334306f; // ~89 deg to avoid flip singularities
    if (p < -limit) return -limit;
    if (p > limit) return limit;
    return p;
}

float sample_field(const GridField &field, float fx, float fy) {
    int x = static_cast<int>(fx);
    int y = static_cast<int>(fy);
    if (x < 0 || y < 0 || x >= field.width || y >= field.height) {
        return 0.0f;
    }
    return field.at(x, y);
}

float sample_field(const GridField &field, float fx, float fy, float fz) {
    int x = static_cast<int>(fx);
    int y = static_cast<int>(fy);
    int z = static_cast<int>(fz);
    if (x < 0 || y < 0 || z < 0 || x >= field.width || y >= field.height || z >= field.depth) {
        return 0.0f;
    }
    return field.at(x, y, z);
}

float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float species_color_pref(int species) {
    switch (species) {
        case 0: return 1.0f;  // warm tribe
        case 1: return -1.0f; // cool tribe
        default: return 0.0f; // neutral/contrast tribes
    }
}

float composition_score(float x, float y, int width, int height) {
    if (width <= 1 || height <= 1) return 0.0f;
    const float phi = 0.6180339887f;
    const float inv = 1.0f - phi;
    float nx = x / static_cast<float>(width - 1);
    float ny = y / static_cast<float>(height - 1);

    float dx = std::min(std::abs(nx - phi), std::abs(nx - inv));
    float dy = std::min(std::abs(ny - phi), std::abs(ny - inv));
    float line_dist = std::min(dx, dy);
    float line_score = 1.0f - clamp01(line_dist / 0.25f);

    float best = 10.0f;
    float points[2] = {inv, phi};
    for (float px : points) {
        for (float py : points) {
            float ddx = nx - px;
            float ddy = ny - py;
            float dist = std::sqrt(ddx * ddx + ddy * ddy);
            if (dist < best) best = dist;
        }
    }
    float point_score = 1.0f - clamp01(best / 0.35f);

    return 0.6f * line_score + 0.4f * point_score;
}

float composition_score_3d(float x, float y, float z, int width, int height, int depth) {
    if (width <= 1 || height <= 1 || depth <= 1) return 0.0f;
    const float phi = 0.6180339887f;
    const float inv = 1.0f - phi;
    float nx = x / static_cast<float>(width - 1);
    float ny = y / static_cast<float>(height - 1);
    float nz = z / static_cast<float>(depth - 1);

    float dx = std::min(std::abs(nx - phi), std::abs(nx - inv));
    float dy = std::min(std::abs(ny - phi), std::abs(ny - inv));
    float dz = std::min(std::abs(nz - phi), std::abs(nz - inv));
    float line_dist = std::min(dx, std::min(dy, dz));
    float line_score = 1.0f - clamp01(line_dist / 0.25f);

    float best = 10.0f;
    float points[2] = {inv, phi};
    for (float px : points) {
        for (float py : points) {
            for (float pz : points) {
                float ddx = nx - px;
                float ddy = ny - py;
                float ddz = nz - pz;
                float dist = std::sqrt(ddx * ddx + ddy * ddy + ddz * ddz);
                if (dist < best) best = dist;
            }
        }
    }
    float point_score = 1.0f - clamp01(best / 0.45f);

    return 0.55f * line_score + 0.45f * point_score;
}

void direction_from_angles(float yaw, float pitch, float &dx, float &dy, float &dz) {
    float cp = std::cos(pitch);
    dx = cp * std::cos(yaw);
    dy = cp * std::sin(yaw);
    dz = std::sin(pitch);
}
} // namespace

void Agent::step(Rng &rng,
                 const SimParams &params,
                 int fitness_window,
                 const SpeciesProfile &profile,
                 GridField &phero_food,
                 GridField &phero_danger,
                 GridField &molecules,
                 GridField &resources,
                 const GridField &mycel) {
    last_energy = energy;
    const float sensor = params.agent_sense_radius * genome.sense_gain;
    const float turn = params.agent_random_turn * profile.exploration_mul;

    bool bounced = false;
    if (phero_food.depth <= 1) {
        float angles[3] = {
            heading - 0.6f,
            heading,
            heading + 0.6f
        };
        float weights[3] = {};

        for (int i = 0; i < 3; ++i) {
            float nx = x + std::cos(angles[i]) * sensor;
            float ny = y + std::sin(angles[i]) * sensor;
            float p_food = sample_field(phero_food, nx, ny) * genome.pheromone_gain * profile.food_attraction_mul;
            float p_danger = sample_field(phero_danger, nx, ny) * genome.pheromone_gain * profile.danger_aversion_mul;
            float r = sample_field(resources, nx, ny) * profile.resource_weight_mul;
            float m_raw = sample_field(molecules, nx, ny);
            float pref = species_color_pref(species);
            float m_pref = (pref != 0.0f) ? (m_raw * pref) : (std::fabs(m_raw) * 0.5f);
            float m = m_pref * profile.molecule_weight_mul;
            float my = sample_field(mycel, nx, ny) * profile.mycel_attraction_mul * (1.0f + genome.mycel_drive);
            float signal = p_food + p_danger + my;
            float novelty = 1.0f - std::min(1.0f, std::max(0.0f, signal));
            float soften = genome.soften_bias * (1.0f - r);
            float w = p_food + r + 0.25f * m + my + profile.novelty_weight * novelty + soften;
            w -= p_danger;
            if (genome.edge_seek > 0.0f) {
                w += p_danger * genome.edge_seek * 1.2f;
            } else if (genome.edge_seek < 0.0f) {
                w -= p_danger * (-genome.edge_seek) * 0.5f;
            }
            if (w < 0.001f) w = 0.001f;
            weights[i] = w;
        }

        float total = weights[0] + weights[1] + weights[2];
        float pick = rng.uniform(0.0f, total);
        int choice = 1;
        for (int i = 0; i < 3; ++i) {
            if (pick <= weights[i]) {
                choice = i;
                break;
            }
            pick -= weights[i];
        }

        heading = wrap_angle(angles[choice] + rng.uniform(-turn, turn) * genome.exploration_bias);

        float nx = x + std::cos(heading);
        float ny = y + std::sin(heading);

        if (nx >= 0.0f && ny >= 0.0f && nx < phero_food.width && ny < phero_food.height) {
            x = nx;
            y = ny;
        } else {
            heading = wrap_angle(heading + 3.1415926f);
            bounced = true;
        }

        int cx = static_cast<int>(x);
        int cy = static_cast<int>(y);
        if (cx >= 0 && cy >= 0 && cx < resources.width && cy < resources.height) {
            float &cell = resources.at(cx, cy);
            float harvested = std::min(cell, params.agent_harvest);
            cell -= harvested;
            energy += harvested;

            float deposit = params.phero_food_deposit_scale * harvested;
            phero_food.at(cx, cy) += deposit * profile.deposit_food_mul;
            molecules.at(cx, cy) += harvested * 0.5f;
        }
    } else {
        float yaw_options[5] = {
            heading,
            heading - 0.6f,
            heading + 0.6f,
            heading,
            heading
        };
        float pitch_options[5] = {
            pitch,
            pitch,
            pitch,
            pitch + 0.45f,
            pitch - 0.45f
        };
        float weights[5] = {};

        for (int i = 0; i < 5; ++i) {
            float dx = 0.0f, dy = 0.0f, dz = 0.0f;
            direction_from_angles(yaw_options[i], pitch_options[i], dx, dy, dz);
            float nx = x + dx * sensor;
            float ny = y + dy * sensor;
            float nz = z + dz * sensor;
            float p_food = sample_field(phero_food, nx, ny, nz) * genome.pheromone_gain * profile.food_attraction_mul;
            float p_danger = sample_field(phero_danger, nx, ny, nz) * genome.pheromone_gain * profile.danger_aversion_mul;
            float r = sample_field(resources, nx, ny, nz) * profile.resource_weight_mul;
            float m_raw = sample_field(molecules, nx, ny, nz);
            float pref = species_color_pref(species);
            float m_pref = (pref != 0.0f) ? (m_raw * pref) : (std::fabs(m_raw) * 0.5f);
            float m = m_pref * profile.molecule_weight_mul;
            float my = sample_field(mycel, nx, ny, nz) * profile.mycel_attraction_mul * (1.0f + genome.mycel_drive);
            float signal = p_food + p_danger + my;
            float novelty = 1.0f - std::min(1.0f, std::max(0.0f, signal));
            float soften = genome.soften_bias * (1.0f - r);
            float w = p_food + r + 0.25f * m + my + profile.novelty_weight * novelty + soften;
            w -= p_danger;
            if (genome.edge_seek > 0.0f) {
                w += p_danger * genome.edge_seek * 1.2f;
            } else if (genome.edge_seek < 0.0f) {
                w -= p_danger * (-genome.edge_seek) * 0.5f;
            }
            if (w < 0.001f) w = 0.001f;
            weights[i] = w;
        }

        float total = 0.0f;
        for (float w : weights) total += w;
        float pick = rng.uniform(0.0f, total);
        int choice = 0;
        for (int i = 0; i < 5; ++i) {
            if (pick <= weights[i]) {
                choice = i;
                break;
            }
            pick -= weights[i];
        }

        heading = wrap_angle(yaw_options[choice] + rng.uniform(-turn, turn) * genome.exploration_bias);
        pitch = clamp_pitch(pitch_options[choice] + rng.uniform(-turn, turn) * genome.exploration_bias);

        float dx = 0.0f, dy = 0.0f, dz = 0.0f;
        direction_from_angles(heading, pitch, dx, dy, dz);
        float nx = x + dx;
        float ny = y + dy;
        float nz = z + dz;

        if (nx >= 0.0f && ny >= 0.0f && nz >= 0.0f &&
            nx < phero_food.width && ny < phero_food.height && nz < phero_food.depth) {
            x = nx;
            y = ny;
            z = nz;
        } else {
            heading = wrap_angle(heading + 3.1415926f);
            pitch = clamp_pitch(-pitch);
            bounced = true;
        }

        int cx = static_cast<int>(x);
        int cy = static_cast<int>(y);
        int cz = static_cast<int>(z);
        if (cx >= 0 && cy >= 0 && cz >= 0 &&
            cx < resources.width && cy < resources.height && cz < resources.depth) {
            float &cell = resources.at(cx, cy, cz);
            float harvested = std::min(cell, params.agent_harvest);
            cell -= harvested;
            energy += harvested;

            float deposit = params.phero_food_deposit_scale * harvested;
            phero_food.at(cx, cy, cz) += deposit * profile.deposit_food_mul;
            molecules.at(cx, cy, cz) += harvested * 0.5f;
        }
    }

    energy -= params.agent_move_cost;
    if (energy < 0.0f) {
        energy = 0.0f;
    }

    float delta = energy - last_energy;
    if (delta > 0.0f) {
        fitness_accum += delta;
    }
    float chroma = (resources.depth <= 1) ? sample_field(molecules, x, y) : sample_field(molecules, x, y, z);
    float comp_score = (resources.depth <= 1)
                           ? composition_score(x, y, resources.width, resources.height)
                           : composition_score_3d(x, y, z, resources.width, resources.height, resources.depth);
    float pref = species_color_pref(species);
    float tribe_score = 0.0f;
    if (pref > 0.0f) {
        tribe_score = std::max(0.0f, chroma);
    } else if (pref < 0.0f) {
        tribe_score = std::max(0.0f, -chroma);
    } else {
        tribe_score = std::fabs(chroma);
    }
    fitness_accum += 0.03f * comp_score + 0.04f * clamp01(tribe_score);
    fitness_ticks += 1;
    if (fitness_window > 0 && fitness_ticks >= fitness_window) {
        fitness_value = fitness_accum / static_cast<float>(fitness_ticks);
        fitness_accum = 0.0f;
        fitness_ticks = 0;
    }

    float danger_deposit = 0.0f;
    if (bounced) {
        danger_deposit += params.danger_bounce_deposit;
    }
    if (delta < -params.danger_delta_threshold) {
        danger_deposit += (-delta) * params.phero_danger_deposit_scale;
    }
    if (danger_deposit > 0.0f) {
        int dx = static_cast<int>(x);
        int dy = static_cast<int>(y);
        int dz = static_cast<int>(z);
        if (dx >= 0 && dy >= 0 && dz >= 0 &&
            dx < phero_danger.width && dy < phero_danger.height && dz < phero_danger.depth) {
            phero_danger.at(dx, dy, dz) += danger_deposit * profile.deposit_danger_mul;
        }
    }

    if (profile.counter_deposit_mul > 0.0f) {
        int dx = static_cast<int>(x);
        int dy = static_cast<int>(y);
        int dz = static_cast<int>(z);
        if (dx >= 0 && dy >= 0 && dz >= 0 &&
            dx < phero_food.width && dy < phero_food.height && dz < phero_food.depth) {
            float local_food = phero_food.at(dx, dy, dz);
            float local_mycel = (mycel.depth <= 1)
                                    ? sample_field(mycel, static_cast<float>(dx), static_cast<float>(dy))
                                    : sample_field(mycel, static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
            float density = local_food + local_mycel;
            if (density > profile.over_density_threshold) {
                float reduction = (density - profile.over_density_threshold) * profile.counter_deposit_mul;
                phero_food.at(dx, dy, dz) = std::max(0.0f, local_food - reduction);
            }
        }
    }
}
