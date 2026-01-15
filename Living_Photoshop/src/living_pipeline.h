#pragma once

#include <string>

struct LivingOptions {
    int steps = 400;
    int agents = 1500;
    int seed = 42;
    float blur_sigma = 1.2f;
    float unsharp_sigma = 1.0f;
    float unsharp_amount = 1.2f;
    float unsharp_threshold = 0.02f;
    float mutate_strength = 0.05f;
    float mycel_strength = 0.15f;
    float edge_gain = 1.4f;
    float blur_threshold = 0.35f;
    float blur_gain = 1.0f;
    int aa_samples = 4;
    int size = 0;
    int loop_damp_start = 3;
    float loop_damp = 0.9f;
    std::string command_file;
    std::string dna_import;
    std::string dna_export;
    std::string dna_bootstrap;
    std::string dna_style;
    bool eternal = false;
    int eternal_loops = 10;
};

int run_living_pipeline(const std::string &input_path, const std::string &output_path, const LivingOptions &opt, std::string &error);
