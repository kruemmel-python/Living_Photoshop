#include "mycel.h"

#include <algorithm>

MycelNetwork::MycelNetwork(int w, int h) : density(w, h, 0.0f), width(w), height(h), depth(1) {}

MycelNetwork::MycelNetwork(int w, int h, int d)
    : density(w, h, std::max(1, d), 0.0f),
      width(w),
      height(h),
      depth(std::max(1, d)) {}

void MycelNetwork::update(const SimParams &params, const GridField &pheromone, const GridField &resources) {
    std::vector<float> next(density.data.size(), 0.0f);

    auto clamp01 = [](float v) {
        return std::max(0.0f, std::min(1.0f, v));
    };

    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float current = density.at(x, y, z);
                float local_pheromone = pheromone.at(x, y, z);
                float local_resource = resources.at(x, y, z);

                float drive = params.mycel_drive_p * local_pheromone + params.mycel_drive_r * local_resource;
                drive = clamp01(drive);
                float threshold = params.mycel_drive_threshold;
                if (drive > threshold) {
                    drive = (drive - threshold) / (1.0f - threshold);
                } else {
                    drive = 0.0f;
                }

                float neighbor_sum = 0.0f;
                int neighbor_count = 0;
                auto add = [&](int nx, int ny, int nz) {
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= width || ny >= height || nz >= depth) {
                        return;
                    }
                    neighbor_sum += density.at(nx, ny, nz);
                    neighbor_count++;
                };

                add(x - 1, y, z);
                add(x + 1, y, z);
                add(x, y - 1, z);
                add(x, y + 1, z);
                add(x, y, z - 1);
                add(x, y, z + 1);

                float neighbor_avg = (neighbor_count > 0) ? (neighbor_sum / static_cast<float>(neighbor_count)) : current;
                float transport = params.mycel_transport * (neighbor_avg - current);
                float growth = params.mycel_growth * drive * (1.0f - current);
                float decay = params.mycel_decay * current;

                float value = current + growth + transport - decay;
                next[density.index(x, y, z)] = clamp01(value);
            }
        }
    }

    density.data.swap(next);
}
