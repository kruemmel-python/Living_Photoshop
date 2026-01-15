#pragma once

#include <cstddef>
#include <vector>

struct VoxelField {
    int width = 0;
    int height = 0;
    int depth = 1;
    std::vector<float> data;

    VoxelField() = default;
    VoxelField(int w, int h, float value = 0.0f);
    VoxelField(int w, int h, int d, float value = 0.0f);

    size_t index(int x, int y, int z) const;

    float &at(int x, int y);
    float at(int x, int y) const;
    float &at(int x, int y, int z);
    float at(int x, int y, int z) const;

    void fill(float value);
};

using GridField = VoxelField;

struct FieldParams {
    float evaporation = 0.0f;
    float diffusion = 0.0f;
};

void diffuse_and_evaporate(GridField &field, const FieldParams &params);
