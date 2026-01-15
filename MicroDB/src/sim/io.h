#pragma once

#include <string>
#include <vector>

struct GridData {
    int width = 0;
    int height = 0;
    std::vector<float> values;
};

struct VolumeData {
    int width = 0;
    int height = 0;
    int depth = 0;
    std::vector<float> values;
};

bool load_grid_csv(const std::string &path, GridData &out, std::string &error);
bool save_grid_csv(const std::string &path, int width, int height, const std::vector<float> &values, std::string &error);
bool load_volume_csv(const std::string &path, VolumeData &out, std::string &error);
bool save_volume_csv(const std::string &path, int width, int height, int depth, const std::vector<float> &values, std::string &error);
