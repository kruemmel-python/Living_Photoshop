#pragma once

#include "fields.h"
#include "params.h"

struct MycelNetwork {
    GridField density;
    int width = 0;
    int height = 0;
    int depth = 1;

    MycelNetwork() = default;
    MycelNetwork(int w, int h);
    MycelNetwork(int w, int h, int d);

    void update(const SimParams &params, const GridField &pheromone, const GridField &resources);
};
