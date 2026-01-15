#include "fields.h"

#include <algorithm>

VoxelField::VoxelField(int w, int h, float value) : VoxelField(w, h, 1, value) {}

VoxelField::VoxelField(int w, int h, int d, float value)
    : width(w), height(h), depth(std::max(1, d)), data(static_cast<size_t>(w) * h * std::max(1, d), value) {}

size_t VoxelField::index(int x, int y, int z) const {
    return static_cast<size_t>(z) * static_cast<size_t>(height) * static_cast<size_t>(width) +
           static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
}

float &VoxelField::at(int x, int y) {
    return data[index(x, y, 0)];
}

float VoxelField::at(int x, int y) const {
    return data[index(x, y, 0)];
}

float &VoxelField::at(int x, int y, int z) {
    return data[index(x, y, z)];
}

float VoxelField::at(int x, int y, int z) const {
    return data[index(x, y, z)];
}

void VoxelField::fill(float value) {
    std::fill(data.begin(), data.end(), value);
}

void diffuse_and_evaporate(GridField &field, const FieldParams &params) {
    std::vector<float> next(field.data.size(), 0.0f);
    const float diff = params.diffusion;
    const float evap = params.evaporation;

    if (field.depth <= 1) {
        for (int y = 0; y < field.height; ++y) {
            for (int x = 0; x < field.width; ++x) {
                float center = field.at(x, y);
                float sum = center * (1.0f - diff);
                int count = 0;

                auto add = [&](int nx, int ny) {
                    if (nx < 0 || ny < 0 || nx >= field.width || ny >= field.height) {
                        return;
                    }
                    sum += field.at(nx, ny) * (diff * 0.25f);
                    count++;
                };

                add(x - 1, y);
                add(x + 1, y);
                add(x, y - 1);
                add(x, y + 1);

                float value = sum;
                if (count < 4) {
                    value = center;
                }
                value *= (1.0f - evap);
                next[static_cast<size_t>(y) * field.width + x] = std::max(0.0f, value);
            }
        }
    } else {
        const float neighbor_weight = diff / 6.0f;
        for (int z = 0; z < field.depth; ++z) {
            for (int y = 0; y < field.height; ++y) {
                for (int x = 0; x < field.width; ++x) {
                    float center = field.at(x, y, z);
                    float sum = center * (1.0f - diff);
                    int count = 0;

                    auto add = [&](int nx, int ny, int nz) {
                        if (nx < 0 || ny < 0 || nz < 0 ||
                            nx >= field.width || ny >= field.height || nz >= field.depth) {
                            return;
                        }
                        sum += field.at(nx, ny, nz) * neighbor_weight;
                        count++;
                    };

                    add(x - 1, y, z);
                    add(x + 1, y, z);
                    add(x, y - 1, z);
                    add(x, y + 1, z);
                    add(x, y, z - 1);
                    add(x, y, z + 1);

                    float value = sum;
                    if (count < 6) {
                        value = center;
                    }
                    value *= (1.0f - evap);
                    next[field.index(x, y, z)] = std::max(0.0f, value);
                }
            }
        }
    }

    field.data.swap(next);
}
