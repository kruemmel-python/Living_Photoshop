#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ImageU8 {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgb; // interleaved RGB
};

bool load_ppm(const std::string &path, ImageU8 &out, std::string &error);
bool save_ppm(const std::string &path, int width, int height, const std::vector<uint8_t> &rgb, std::string &error);
