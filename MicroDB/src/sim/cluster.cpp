#include "cluster.h"

#include <algorithm>

namespace {
int ceil_div(int a, int b) {
    return (a + b - 1) / b;
}
} // namespace

int brick_index(const BrickGrid &grid, int bx, int by, int bz) {
    return (bz * grid.bricks_y + by) * grid.bricks_x + bx;
}

BrickIndex brick_coords(const BrickGrid &grid, int index) {
    BrickIndex out{};
    if (grid.bricks_x <= 0 || grid.bricks_y <= 0 || grid.bricks_z <= 0) {
        return out;
    }
    int plane = grid.bricks_x * grid.bricks_y;
    out.bz = index / plane;
    int rem = index - out.bz * plane;
    out.by = rem / grid.bricks_x;
    out.bx = rem - out.by * grid.bricks_x;
    return out;
}

BrickBounds brick_bounds(const BrickGrid &grid, int bx, int by, int bz) {
    BrickBounds bounds{};
    int bw = ceil_div(grid.volume_width, grid.bricks_x);
    int bh = ceil_div(grid.volume_height, grid.bricks_y);
    int bd = ceil_div(grid.volume_depth, grid.bricks_z);

    int x0 = bx * bw;
    int y0 = by * bh;
    int z0 = bz * bd;
    int x1 = std::min(grid.volume_width, x0 + bw);
    int y1 = std::min(grid.volume_height, y0 + bh);
    int z1 = std::min(grid.volume_depth, z0 + bd);

    bounds.core_x0 = x0;
    bounds.core_y0 = y0;
    bounds.core_z0 = z0;
    bounds.core_x1 = x1;
    bounds.core_y1 = y1;
    bounds.core_z1 = z1;

    bounds.x0 = std::max(0, x0 - grid.ghost);
    bounds.y0 = std::max(0, y0 - grid.ghost);
    bounds.z0 = std::max(0, z0 - grid.ghost);
    bounds.x1 = std::min(grid.volume_width, x1 + grid.ghost);
    bounds.y1 = std::min(grid.volume_height, y1 + grid.ghost);
    bounds.z1 = std::min(grid.volume_depth, z1 + grid.ghost);
    return bounds;
}

bool locate_brick(const BrickGrid &grid, float x, float y, float z, BrickIndex &out) {
    if (grid.volume_width <= 0 || grid.volume_height <= 0 || grid.volume_depth <= 0) {
        return false;
    }
    if (x < 0.0f || y < 0.0f || z < 0.0f ||
        x >= grid.volume_width || y >= grid.volume_height || z >= grid.volume_depth) {
        return false;
    }
    int bw = ceil_div(grid.volume_width, grid.bricks_x);
    int bh = ceil_div(grid.volume_height, grid.bricks_y);
    int bd = ceil_div(grid.volume_depth, grid.bricks_z);
    out.bx = std::min(grid.bricks_x - 1, std::max(0, static_cast<int>(x) / bw));
    out.by = std::min(grid.bricks_y - 1, std::max(0, static_cast<int>(y) / bh));
    out.bz = std::min(grid.bricks_z - 1, std::max(0, static_cast<int>(z) / bd));
    return true;
}

std::unordered_map<int, std::vector<Agent>> collect_migrants(std::vector<Agent> &agents, const BrickGrid &grid, int local_index) {
    std::unordered_map<int, std::vector<Agent>> outgoing;
    BrickIndex local = brick_coords(grid, local_index);
    BrickBounds bounds = brick_bounds(grid, local.bx, local.by, local.bz);

    auto in_core = [&](const Agent &a) {
        return a.x >= bounds.core_x0 && a.y >= bounds.core_y0 && a.z >= bounds.core_z0 &&
               a.x < bounds.core_x1 && a.y < bounds.core_y1 && a.z < bounds.core_z1;
    };

    for (auto it = agents.begin(); it != agents.end(); ) {
        if (in_core(*it)) {
            ++it;
            continue;
        }
        BrickIndex target{};
        if (locate_brick(grid, it->x, it->y, it->z, target)) {
            int target_index = brick_index(grid, target.bx, target.by, target.bz);
            outgoing[target_index].push_back(*it);
        }
        it = agents.erase(it);
    }
    return outgoing;
}

void apply_migrants(std::vector<Agent> &agents, const std::vector<Agent> &incoming) {
    agents.insert(agents.end(), incoming.begin(), incoming.end());
}

std::vector<float> extract_region(const GridField &field, int x0, int y0, int z0, int w, int h, int d) {
    std::vector<float> out;
    if (w <= 0 || h <= 0 || d <= 0) return out;
    if (x0 < 0 || y0 < 0 || z0 < 0 || x0 + w > field.width || y0 + h > field.height || z0 + d > field.depth) {
        return out;
    }
    out.reserve(static_cast<size_t>(w) * h * d);
    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                out.push_back(field.at(x0 + x, y0 + y, z0 + z));
            }
        }
    }
    return out;
}

void write_region(GridField &field, int x0, int y0, int z0, int w, int h, int d, const std::vector<float> &values) {
    if (w <= 0 || h <= 0 || d <= 0) return;
    if (x0 < 0 || y0 < 0 || z0 < 0 || x0 + w > field.width || y0 + h > field.height || z0 + d > field.depth) {
        return;
    }
    if (static_cast<int>(values.size()) < w * h * d) return;
    int idx = 0;
    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                field.at(x0 + x, y0 + y, z0 + z) = values[static_cast<size_t>(idx++)];
            }
        }
    }
}
