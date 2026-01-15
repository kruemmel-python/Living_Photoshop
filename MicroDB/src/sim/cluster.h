#pragma once

#include <unordered_map>
#include <vector>

#include "agent.h"
#include "fields.h"

struct BrickGrid {
    int volume_width = 0;
    int volume_height = 0;
    int volume_depth = 0;
    int bricks_x = 1;
    int bricks_y = 1;
    int bricks_z = 1;
    int ghost = 0;
};

struct BrickIndex {
    int bx = 0;
    int by = 0;
    int bz = 0;
};

struct BrickBounds {
    int x0 = 0;
    int y0 = 0;
    int z0 = 0;
    int x1 = 0;
    int y1 = 0;
    int z1 = 0;
    int core_x0 = 0;
    int core_y0 = 0;
    int core_z0 = 0;
    int core_x1 = 0;
    int core_y1 = 0;
    int core_z1 = 0;
};

struct SwarmPacket {
    int target_node_id = -1;
    std::vector<Agent> migrating_agents;
    std::vector<float> boundary_pheromones;
    int boundary_width = 0;
    int boundary_height = 0;
    int boundary_depth = 0;
};

int brick_index(const BrickGrid &grid, int bx, int by, int bz);
BrickIndex brick_coords(const BrickGrid &grid, int index);
BrickBounds brick_bounds(const BrickGrid &grid, int bx, int by, int bz);
bool locate_brick(const BrickGrid &grid, float x, float y, float z, BrickIndex &out);

std::unordered_map<int, std::vector<Agent>> collect_migrants(std::vector<Agent> &agents, const BrickGrid &grid, int local_index);
void apply_migrants(std::vector<Agent> &agents, const std::vector<Agent> &incoming);

std::vector<float> extract_region(const GridField &field, int x0, int y0, int z0, int w, int h, int d);
void write_region(GridField &field, int x0, int y0, int z0, int w, int h, int d, const std::vector<float> &values);
