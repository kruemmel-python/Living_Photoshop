#include "living_pipeline.h"

#include "halo_api.h"
#include "image_io.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "micro_swarm_api.h"

namespace {
struct FocusState {
    bool active = false;
    int x = 0;
    int y = 0;
    int radius = 0;
};

struct SqlCommand {
    enum Kind { NONE, DELETE, SELECT_COUNT } kind = NONE;
    std::string field;
    char op = 0;
    float value = 0.0f;
};

float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void normalize_field(std::vector<float> &v) {
    if (v.empty()) return;
    float mn = v[0];
    float mx = v[0];
    for (float x : v) {
        mn = std::min(mn, x);
        mx = std::max(mx, x);
    }
    float denom = (mx - mn);
    if (denom < 1e-8f) {
        std::fill(v.begin(), v.end(), 0.0f);
        return;
    }
    for (float &x : v) {
        x = (x - mn) / denom;
    }
}

void rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v) {
    float maxv = std::max(r, std::max(g, b));
    float minv = std::min(r, std::min(g, b));
    v = maxv;
    float delta = maxv - minv;
    s = (maxv <= 1e-6f) ? 0.0f : delta / maxv;
    if (delta <= 1e-6f) {
        h = 0.0f;
        return;
    }
    if (maxv == r) {
        h = (g - b) / delta;
    } else if (maxv == g) {
        h = 2.0f + (b - r) / delta;
    } else {
        h = 4.0f + (r - g) / delta;
    }
    h /= 6.0f;
    if (h < 0.0f) h += 1.0f;
}

float hue_distance(float a, float b) {
    float d = std::fabs(a - b);
    return (d > 0.5f) ? (1.0f - d) : d;
}

float harmony_score(float hue, float sat, float base_hue) {
    if (sat < 0.02f) return 0.0f;
    float analog_w = 0.08f;
    float comp_w = 0.10f;
    float triad_w = 0.12f;
    float analog_targets[3] = {
        base_hue,
        std::fmod(base_hue + 1.0f / 12.0f, 1.0f),
        std::fmod(base_hue + 11.0f / 12.0f, 1.0f)
    };
    float comp_target = std::fmod(base_hue + 0.5f, 1.0f);
    float triad_targets[2] = {
        std::fmod(base_hue + 1.0f / 3.0f, 1.0f),
        std::fmod(base_hue + 2.0f / 3.0f, 1.0f)
    };
    float best = 0.0f;
    for (float t : analog_targets) {
        float d = hue_distance(hue, t);
        best = std::max(best, 1.0f - clamp01(d / analog_w));
    }
    {
        float d = hue_distance(hue, comp_target);
        best = std::max(best, 1.0f - clamp01(d / comp_w));
    }
    for (float t : triad_targets) {
        float d = hue_distance(hue, t);
        best = std::max(best, 1.0f - clamp01(d / triad_w));
    }
    return best * clamp01(sat);
}

float hue_band_score(float hue, float center, float width) {
    float d = hue_distance(hue, center);
    return 1.0f - clamp01(d / width);
}

void resize_rgb_bilinear(const std::vector<float> &src, int sw, int sh, std::vector<float> &dst, int dw, int dh) {
    dst.assign((size_t)dw * dh * 3, 0.0f);
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
    float scale_x = (dw > 1) ? (static_cast<float>(sw - 1) / static_cast<float>(dw - 1)) : 0.0f;
    float scale_y = (dh > 1) ? (static_cast<float>(sh - 1) / static_cast<float>(dh - 1)) : 0.0f;
    for (int y = 0; y < dh; ++y) {
        float sy = scale_y * y;
        int y0 = static_cast<int>(sy);
        int y1 = std::min(y0 + 1, sh - 1);
        float fy = sy - y0;
        for (int x = 0; x < dw; ++x) {
            float sx = scale_x * x;
            int x0 = static_cast<int>(sx);
            int x1 = std::min(x0 + 1, sw - 1);
            float fx = sx - x0;
            size_t i00 = (static_cast<size_t>(y0) * sw + x0) * 3;
            size_t i01 = (static_cast<size_t>(y0) * sw + x1) * 3;
            size_t i10 = (static_cast<size_t>(y1) * sw + x0) * 3;
            size_t i11 = (static_cast<size_t>(y1) * sw + x1) * 3;
            size_t odx = (static_cast<size_t>(y) * dw + x) * 3;
            for (int c = 0; c < 3; ++c) {
                float v00 = src[i00 + c];
                float v01 = src[i01 + c];
                float v10 = src[i10 + c];
                float v11 = src[i11 + c];
                float vx0 = v00 + (v01 - v00) * fx;
                float vx1 = v10 + (v11 - v10) * fx;
                dst[odx + c] = vx0 + (vx1 - vx0) * fy;
            }
        }
    }
}

void compute_color_tribe_field(const std::vector<float> &rgb_f32, int width, int height, std::vector<float> &out) {
    out.assign((size_t)width * height, 0.0f);
    double sum_cos = 0.0;
    double sum_sin = 0.0;
    double sum_w = 0.0;
    for (int i = 0; i < width * height; ++i) {
        float h, s, v;
        rgb_to_hsv(rgb_f32[i * 3 + 0], rgb_f32[i * 3 + 1], rgb_f32[i * 3 + 2], h, s, v);
        double w = static_cast<double>(s);
        sum_cos += std::cos(h * 6.283185307) * w;
        sum_sin += std::sin(h * 6.283185307) * w;
        sum_w += w;
    }
    float base_hue = 0.0f;
    if (sum_w > 1e-6) {
        double ang = std::atan2(sum_sin, sum_cos);
        if (ang < 0.0) ang += 6.283185307;
        base_hue = static_cast<float>(ang / 6.283185307);
    }
    const float warm_center = 1.0f / 12.0f; // ~30 deg
    const float cool_center = 7.0f / 12.0f; // ~210 deg
    const float band_width = 0.25f;

    for (int i = 0; i < width * height; ++i) {
        float h, s, v;
        rgb_to_hsv(rgb_f32[i * 3 + 0], rgb_f32[i * 3 + 1], rgb_f32[i * 3 + 2], h, s, v);
        float warm = hue_band_score(h, warm_center, band_width);
        float cool = hue_band_score(h, cool_center, band_width);
        float harmony = harmony_score(h, s, base_hue);
        float mag = clamp01(s) * (0.5f + 0.5f * clamp01(v)) * (0.6f + 0.4f * harmony);
        float value = (warm - cool) * mag;
        out[i] = std::max(-1.0f, std::min(1.0f, value));
    }
}

bool compute_luma_gradient(const std::vector<float> &rgb_f32, int width, int height,
                           std::vector<float> &luma, std::vector<float> &gradient, std::string &error) {
    luma.assign((size_t)width * height, 0.0f);
    for (int i = 0; i < width * height; ++i) {
        float r = rgb_f32[i * 3 + 0];
        float g = rgb_f32[i * 3 + 1];
        float b = rgb_f32[i * 3 + 2];
        luma[i] = 0.299f * r + 0.587f * g + 0.114f * b;
    }

    gradient.assign((size_t)width * height, 0.0f);
    if (halo_sobel_f32(luma.data(), (long long)width * 4,
                       gradient.data(), (long long)width * 4,
                       width, height, 1) != 0) {
        error = "HALO sobel fehlgeschlagen";
        return false;
    }

    normalize_field(luma);
    normalize_field(gradient);
    return true;
}

std::string loop_output_path(const std::string &base, int loop_index, int total_loops, bool eternal) {
    if (!eternal) return base;
    if (total_loops <= 1) return base;
    size_t dot = base.find_last_of('.');
    std::string prefix = (dot == std::string::npos) ? base : base.substr(0, dot);
    std::string suffix = (dot == std::string::npos) ? "" : base.substr(dot);
    std::ostringstream oss;
    oss << prefix << "_loop" << loop_index + 1 << suffix;
    return oss.str();
}

bool parse_sql_line(const std::string &line, SqlCommand &cmd) {
    cmd = SqlCommand{};
    std::string s = line;
    for (char &c : s) c = (char)std::tolower((unsigned char)c);
    std::istringstream iss(s);
    std::string token;
    iss >> token;
    if (token != "sql") return false;
    iss >> token;
    if (token == "delete") {
        cmd.kind = SqlCommand::DELETE;
    } else if (token == "select") {
        cmd.kind = SqlCommand::SELECT_COUNT;
        iss >> token; // count
    } else {
        return false;
    }
    iss >> token; // from
    iss >> token; // pixels
    iss >> token; // where
    if (token != "where") {
        return false;
    }
    iss >> cmd.field;
    iss >> token;
    if (token != "<" && token != ">") {
        return false;
    }
    cmd.op = token[0];
    iss >> cmd.value;
    return true;
}

float eval_condition(float value, char op, float threshold) {
    if (op == '<') return value < threshold ? 1.0f : 0.0f;
    if (op == '>') return value > threshold ? 1.0f : 0.0f;
    return 0.0f;
}

void apply_sql_commands(const std::string &path,
                        const std::vector<float> &energy,
                        const std::vector<float> &resource,
                        const std::vector<float> &danger,
                        const std::vector<float> &mycel,
                        std::vector<float> &delete_mask,
                        FocusState &focus) {
    if (path.empty()) return;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[warn] Konnte Command-Datei nicht oeffnen: " << path << "\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        std::string lower = line;
        for (char &c : lower) c = (char)std::tolower((unsigned char)c);
        std::istringstream iss(lower);
        std::string token;
        iss >> token;
        if (token == "goto") {
            iss >> focus.x >> focus.y >> focus.radius;
            focus.active = true;
            continue;
        }
        SqlCommand cmd;
        if (!parse_sql_line(line, cmd)) {
            std::cerr << "[warn] Unbekanntes Kommando: " << line << "\n";
            continue;
        }
        const std::vector<float> *field = nullptr;
        if (cmd.field == "energy") field = &energy;
        else if (cmd.field == "resource") field = &resource;
        else if (cmd.field == "danger") field = &danger;
        else if (cmd.field == "mycel") field = &mycel;
        else {
            std::cerr << "[warn] Unbekanntes Feld: " << cmd.field << "\n";
            continue;
        }
        int count = 0;
        for (size_t i = 0; i < field->size(); ++i) {
            float hit = eval_condition((*field)[i], cmd.op, cmd.value);
            if (cmd.kind == SqlCommand::DELETE) {
                if (hit > 0.0f) {
                    delete_mask[i] = 1.0f;
                    count++;
                }
            } else if (cmd.kind == SqlCommand::SELECT_COUNT) {
                if (hit > 0.0f) count++;
            }
        }
        if (cmd.kind == SqlCommand::SELECT_COUNT) {
            std::cout << "[sql] count=" << count << "\n";
        } else {
            std::cout << "[sql] deleted=" << count << "\n";
        }
    }
}

void split_rgb(const std::vector<float> &rgb, int width, int height,
               std::vector<float> &r, std::vector<float> &g, std::vector<float> &b) {
    r.resize((size_t)width * height);
    g.resize((size_t)width * height);
    b.resize((size_t)width * height);
    for (int i = 0; i < width * height; ++i) {
        r[i] = rgb[i * 3 + 0];
        g[i] = rgb[i * 3 + 1];
        b[i] = rgb[i * 3 + 2];
    }
}

void merge_rgb(std::vector<float> &rgb, int width, int height,
               const std::vector<float> &r, const std::vector<float> &g, const std::vector<float> &b) {
    rgb.resize((size_t)width * height * 3);
    for (int i = 0; i < width * height; ++i) {
        rgb[i * 3 + 0] = r[i];
        rgb[i * 3 + 1] = g[i];
        rgb[i * 3 + 2] = b[i];
    }
}

void default_params(ms_params_t &p) {
    p.width = 128;
    p.height = 128;
    p.depth = 1;
    p.agent_count = 512;
    p.steps = 200;
    p.pheromone_evaporation = 0.02f;
    p.pheromone_diffusion = 0.15f;
    p.molecule_evaporation = 0.35f;
    p.molecule_diffusion = 0.25f;
    p.resource_regen = 0.0015f;
    p.resource_max = 1.0f;
    p.mycel_decay = 0.003f;
    p.mycel_growth = 0.02f;
    p.mycel_transport = 0.12f;
    p.mycel_drive_threshold = 0.08f;
    p.mycel_drive_p = 0.6f;
    p.mycel_drive_r = 0.4f;
    p.agent_move_cost = 0.01f;
    p.agent_harvest = 0.04f;
    p.agent_deposit_scale = 0.8f;
    p.agent_sense_radius = 2.5f;
    p.agent_random_turn = 0.2f;
    p.dna_capacity = 256;
    p.dna_global_capacity = 128;
    p.dna_survival_bias = 0.7f;
    p.phero_food_deposit_scale = 0.8f;
    p.phero_danger_deposit_scale = 0.6f;
    p.danger_delta_threshold = 0.05f;
    p.danger_bounce_deposit = 0.02f;
    p.evo_enable = 1;
    p.evo_elite_frac = 0.1f;
    p.evo_min_energy_to_store = 1.0f;
    p.evo_mutation_sigma = 0.08f;
    p.evo_exploration_delta = 0.2f;
    p.evo_fitness_window = 24;
    p.evo_age_decay = 0.985f;
    p.global_spawn_frac = 0.15f;
}

struct SwarmResult {
    std::vector<float> mycel;
    ms_style_dna_t style{};
};

bool run_swarm_pass(const std::vector<float> &luma,
                    const std::vector<float> &gradient,
                    const std::vector<float> &harmony,
                    int width,
                    int height,
                    const LivingOptions &opt,
                    const std::string &dna_import,
                    const std::string &dna_export,
                    const std::string &dna_style,
                    SwarmResult &out,
                    std::string &error) {
    ms_config_t cfg{};
    default_params(cfg.params);
    cfg.params.width = width;
    cfg.params.height = height;
    cfg.params.agent_count = opt.agents;
    cfg.params.steps = opt.steps;
    cfg.seed = (uint32_t)opt.seed;

    ms_handle_t *swarm = ms_create(&cfg);
    if (!swarm) {
        error = "MicroDB swarm konnte nicht erstellt werden";
        return false;
    }

    if (!dna_style.empty()) {
        ms_set_dna_style(swarm, dna_style.c_str());
    }

    if (!dna_import.empty()) {
        if (!ms_import_dna_csv(swarm, dna_import.c_str())) {
            std::cerr << "[warn] DNA Import fehlgeschlagen: " << dna_import << "\n";
        }
    }

    ms_copy_field_in(swarm, MS_FIELD_RESOURCES, luma.data(), width * height);
    ms_copy_field_in(swarm, MS_FIELD_PHEROMONE_DANGER, gradient.data(), width * height);
    ms_copy_field_in(swarm, MS_FIELD_PHEROMONE_FOOD, luma.data(), width * height);
    if (!harmony.empty()) {
        ms_copy_field_in(swarm, MS_FIELD_MOLECULES, harmony.data(), width * height);
    }

    ms_run(swarm, opt.steps);

    if (!dna_export.empty()) {
        if (!ms_export_dna_csv(swarm, dna_export.c_str())) {
            std::cerr << "[warn] DNA Export fehlgeschlagen: " << dna_export << "\n";
        }
    }

    out.mycel.assign((size_t)width * height, 0.0f);
    ms_copy_field_out(swarm, MS_FIELD_MYCEL, out.mycel.data(), width * height);
    ms_get_style_dna(swarm, -1, &out.style);
    ms_destroy(swarm);
    return true;
}

}

int run_living_pipeline(const std::string &input_path, const std::string &output_path, const LivingOptions &opt, std::string &error) {
    if (halo_init_features() != 0) {
        error = "HALO init fehlgeschlagen";
        return -1;
    }

    ImageU8 img;
    if (!load_ppm(input_path, img, error)) {
        return -2;
    }

    int width = img.width;
    int height = img.height;

    std::vector<float> rgb_f32((size_t)width * height * 3, 0.0f);
    if (halo_img_rgb_u8_to_f32_interleaved(
            img.rgb.data(), (long long)width * 3,
            rgb_f32.data(), (long long)width * 3 * 4,
            width, height,
            1.0f / 255.0f, 0.0f, 0.0f, 1.0f,
            1) != 0) {
        error = "HALO rgb->f32 fehlgeschlagen";
        return -3;
    }

    if (opt.size > 0 && (width != opt.size || height != opt.size)) {
        std::vector<float> resized;
        resize_rgb_bilinear(rgb_f32, width, height, resized, opt.size, opt.size);
        rgb_f32.swap(resized);
        width = opt.size;
        height = opt.size;
    }

    std::vector<float> luma;
    std::vector<float> gradient;
    std::vector<float> harmony;

    if (!compute_luma_gradient(rgb_f32, width, height, luma, gradient, error)) {
        return -4;
    }
    compute_color_tribe_field(rgb_f32, width, height, harmony);

    std::string dna_import = opt.dna_import;
    std::string dna_export = opt.dna_export;

    if (!opt.dna_bootstrap.empty()) {
        SwarmResult warmup;
        if (!run_swarm_pass(luma, gradient, harmony, width, height, opt, std::string(), opt.dna_bootstrap, opt.dna_style, warmup, error)) {
            return -5;
        }
        if (dna_import.empty()) {
            dna_import = opt.dna_bootstrap;
        }
    }

    if (opt.eternal && dna_import.empty() && !dna_export.empty()) {
        dna_import = dna_export;
    }

    int total_loops = opt.eternal ? opt.eternal_loops : 1;
    int loops = opt.eternal ? (opt.eternal_loops > 0 ? opt.eternal_loops : 10) : 1;

    for (int loop = 0; loop < loops; ++loop) {
        if (!compute_luma_gradient(rgb_f32, width, height, luma, gradient, error)) {
            return -4;
        }
        compute_color_tribe_field(rgb_f32, width, height, harmony);

        SwarmResult swarm_out;
        if (!run_swarm_pass(luma, gradient, harmony, width, height, opt, dna_import, dna_export, opt.dna_style, swarm_out, error)) {
            return -5;
        }

        std::vector<float> &mycel = swarm_out.mycel;
        normalize_field(mycel);

        std::vector<float> energy((size_t)width * height, 0.0f);
        for (int i = 0; i < width * height; ++i) {
            energy[i] = 0.5f * gradient[i] + 0.5f * mycel[i];
        }

        std::vector<float> delete_mask((size_t)width * height, 0.0f);
        FocusState focus;
        apply_sql_commands(opt.command_file, energy, luma, gradient, mycel, delete_mask, focus);

        float edge_gain = opt.edge_gain * (1.0f + swarm_out.style.edge_seek * 0.4f);
        float blur_gain = opt.blur_gain * (0.7f + swarm_out.style.blur_pref * 0.6f);
        float sharpen_gain = 0.6f + swarm_out.style.sharpen_pref * 0.8f;
        float blur_threshold = clampf(opt.blur_threshold * (1.1f - swarm_out.style.soften_bias * 0.5f), 0.05f, 0.95f);
        float mycel_strength = opt.mycel_strength * (0.6f + swarm_out.style.mycel_drive * 0.7f);
        float mutate_strength = opt.mutate_strength * (0.5f + swarm_out.style.contrast_pulse * 0.8f);
        float shift_r = swarm_out.style.color_shift_r * 0.15f;
        float shift_g = swarm_out.style.color_shift_g * 0.15f;
        float shift_b = swarm_out.style.color_shift_b * 0.15f;

        if (opt.eternal && opt.loop_damp > 0.0f && opt.loop_damp < 1.0f && opt.loop_damp_start > 0) {
            int start = opt.loop_damp_start - 1;
            if (loop >= start) {
                float damp = std::pow(opt.loop_damp, static_cast<float>(loop - start));
                edge_gain *= damp;
                sharpen_gain *= damp;
                mutate_strength *= damp;
            }
        }

        std::vector<float> r, g, b;
        split_rgb(rgb_f32, width, height, r, g, b);

        std::vector<float> r_blur(r.size()), g_blur(g.size()), b_blur(b.size());
        std::vector<float> r_sharp(r.size()), g_sharp(g.size()), b_sharp(b.size());

        halo_gaussian_blur_f32(r.data(), (long long)width * 4, r_blur.data(), (long long)width * 4,
                               width, height, opt.blur_sigma, 1);
        halo_gaussian_blur_f32(g.data(), (long long)width * 4, g_blur.data(), (long long)width * 4,
                               width, height, opt.blur_sigma, 1);
        halo_gaussian_blur_f32(b.data(), (long long)width * 4, b_blur.data(), (long long)width * 4,
                               width, height, opt.blur_sigma, 1);

        halo_unsharp_mask_f32(r.data(), (long long)width * 4, r_sharp.data(), (long long)width * 4,
                              width, height, opt.unsharp_sigma, opt.unsharp_amount, opt.unsharp_threshold, 1);
        halo_unsharp_mask_f32(g.data(), (long long)width * 4, g_sharp.data(), (long long)width * 4,
                              width, height, opt.unsharp_sigma, opt.unsharp_amount, opt.unsharp_threshold, 1);
        halo_unsharp_mask_f32(b.data(), (long long)width * 4, b_sharp.data(), (long long)width * 4,
                              width, height, opt.unsharp_sigma, opt.unsharp_amount, opt.unsharp_threshold, 1);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                float focus_gain = 1.0f;
                if (focus.active) {
                    float dx = (float)(x - focus.x);
                    float dy = (float)(y - focus.y);
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float t = clampf(1.0f - dist / std::max(1, focus.radius), 0.0f, 1.0f);
                    focus_gain = 0.6f + 0.8f * t;
                }

                float blur_mask = clampf((blur_threshold - luma[idx]) / std::max(1e-4f, blur_threshold), 0.0f, 1.0f);
                blur_mask = clampf(blur_mask * blur_gain * focus_gain, 0.0f, 1.0f);
                float edge_mask = clampf(gradient[idx] * edge_gain + mycel[idx] * 0.3f, 0.0f, 1.0f);
                edge_mask = clampf(edge_mask * sharpen_gain * focus_gain, 0.0f, 1.0f);

                float m = mycel[idx] - 0.5f;
                float mutate = (energy[idx] - 0.5f) * mutate_strength * focus_gain;
                float tex = m * mycel_strength * focus_gain;

                float rr = r[idx];
                float gg = g[idx];
                float bb = b[idx];

                rr = lerp(rr, r_blur[idx], blur_mask);
                gg = lerp(gg, g_blur[idx], blur_mask);
                bb = lerp(bb, b_blur[idx], blur_mask);

                rr = lerp(rr, r_sharp[idx], edge_mask);
                gg = lerp(gg, g_sharp[idx], edge_mask);
                bb = lerp(bb, b_sharp[idx], edge_mask);

                rr += tex + mutate * 0.6f + shift_r;
                gg += tex - mutate * 0.3f + shift_g;
                bb += tex + mutate * 0.4f + shift_b;

                if (delete_mask[idx] > 0.0f) {
                    rr = r_blur[idx];
                    gg = g_blur[idx];
                    bb = b_blur[idx];
                }

                r[idx] = clampf(rr, 0.0f, 1.0f);
                g[idx] = clampf(gg, 0.0f, 1.0f);
                b[idx] = clampf(bb, 0.0f, 1.0f);
            }
        }

        merge_rgb(rgb_f32, width, height, r, g, b);

        std::vector<uint8_t> out_rgb((size_t)width * height * 3);
        for (int i = 0; i < width * height * 3; ++i) {
            float v = clampf(rgb_f32[i], 0.0f, 1.0f);
            out_rgb[i] = (uint8_t)std::lround(v * 255.0f);
        }

        std::string out_path = loop_output_path(output_path, loop, total_loops, opt.eternal);
        if (!save_ppm(out_path, width, height, out_rgb, error)) {
            return -6;
        }

        if (!opt.eternal) {
            break;
        }
    }

    return 0;
}
