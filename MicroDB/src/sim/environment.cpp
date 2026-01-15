#include "environment.h"

#include <algorithm>

Environment::Environment(int w, int h)
    : resources(w, h, 0.0f),
      blocked(static_cast<size_t>(w) * h, 0),
      width(w),
      height(h),
      depth(1) {}

Environment::Environment(int w, int h, int d)
    : resources(w, h, std::max(1, d), 0.0f),
      blocked(static_cast<size_t>(w) * h * std::max(1, d), 0),
      width(w),
      height(h),
      depth(std::max(1, d)) {}

void Environment::seed_resources(Rng &rng) {
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float v = rng.uniform(0.0f, 1.0f);
                resources.at(x, y, z) = (v > 0.98f) ? rng.uniform(0.5f, 1.0f) : 0.0f;
            }
        }
    }
}

void Environment::regenerate(const SimParams &params) {
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                size_t idx = (static_cast<size_t>(z) * height + y) * width + x;
                if (!blocked.empty() && blocked[idx] != 0) {
                    continue;
                }
                float &cell = resources.at(x, y, z);
                cell += params.resource_regen;
                if (cell > params.resource_max) {
                    cell = params.resource_max;
                }
            }
        }
    }
}

void Environment::apply_block_rect(int x, int y, int w, int h) {
    apply_block_box(x, y, 0, w, h, depth);
}

void Environment::apply_block_box(int x, int y, int z, int w, int h, int d) {
    if (w <= 0 || h <= 0 || d <= 0) {
        return;
    }
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int z0 = std::max(0, z);
    int x1 = std::min(width, x + w);
    int y1 = std::min(height, y + h);
    int z1 = std::min(depth, z + d);
    for (int zz = z0; zz < z1; ++zz) {
        for (int yy = y0; yy < y1; ++yy) {
            for (int xx = x0; xx < x1; ++xx) {
                resources.at(xx, yy, zz) = 0.0f;
                if (!blocked.empty()) {
                    size_t idx = (static_cast<size_t>(zz) * height + yy) * width + xx;
                    blocked[idx] = 1;
                }
            }
        }
    }
}

void Environment::shift_hotspots(int dx, int dy) {
    shift_hotspots(dx, dy, 0);
}

void Environment::shift_hotspots(int dx, int dy, int dz) {
    if (width <= 0 || height <= 0 || depth <= 0) {
        return;
    }
    std::vector<float> next(resources.data.size(), 0.0f);
    int sx = ((dx % width) + width) % width;
    int sy = ((dy % height) + height) % height;
    int sz = ((dz % depth) + depth) % depth;
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int nx = (x + sx) % width;
                int ny = (y + sy) % height;
                int nz = (z + sz) % depth;
                next[(static_cast<size_t>(nz) * height + ny) * width + nx] = resources.at(x, y, z);
            }
        }
    }
    resources.data.swap(next);
}
