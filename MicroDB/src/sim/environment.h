#pragma once

#include "fields.h"
#include "params.h"
#include "rng.h"

#include <cstdint>
#include <vector>

struct Environment {
    GridField resources;
    std::vector<uint8_t> blocked;
    int width = 0;
    int height = 0;
    int depth = 1;

    Environment() = default;
    Environment(int w, int h);
    Environment(int w, int h, int d);

    void seed_resources(Rng &rng);
    void regenerate(const SimParams &params);
    void apply_block_rect(int x, int y, int w, int h);
    void apply_block_box(int x, int y, int z, int w, int h, int d);
    void shift_hotspots(int dx, int dy);
    void shift_hotspots(int dx, int dy, int dz);
};
