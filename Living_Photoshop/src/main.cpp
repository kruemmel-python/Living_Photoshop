#include "living_pipeline.h"

#include <iostream>
#include <string>

namespace {
void print_usage() {
    std::cout << "Living_Photoshop - Swarm-Synthese Bildgenerator\n"
              << "\nUsage:\n"
              << "  living_photoshop --input in.ppm --output out.ppm [options]\n\n"
              << "Options:\n"
              << "  --steps N               Swarm steps (default 400)\n"
              << "  --agents N              Anzahl Agenten (default 1500)\n"
              << "  --seed N                RNG seed (default 42)\n"
              << "  --blur-sigma F          Blur sigma (default 1.2)\n"
              << "  --unsharp-sigma F       Unsharp sigma (default 1.0)\n"
              << "  --unsharp-amount F      Unsharp amount (default 1.2)\n"
              << "  --unsharp-threshold F   Unsharp threshold (default 0.02)\n"
              << "  --mutate F              Mutation strength (default 0.05)\n"
              << "  --mycel F               Mycel texture strength (default 0.15)\n"
              << "  --edge-gain F           Edge gain (default 1.4)\n"
              << "  --blur-threshold F      Resource threshold (default 0.35)\n"
              << "  --blur-gain F           Blur gain (default 1.0)\n"
              << "  --size N                Ausgabe-Groesse (quadratisch, z.B. 2048)\n"
              << "  --loop-damp-start N     Damping ab Loop N (default 3)\n"
              << "  --loop-damp F           Detail-Daempfung pro Loop (default 0.9)\n"
              << "  --commands PATH         Command/SQL file\n"
              << "  --dna-import PATH       DNA CSV laden\n"
              << "  --dna-export PATH       DNA CSV speichern\n"
              << "  --dna-style TAG         Style-Tag fuer neue DNA Eintraege\n"
              << "  --demo-bootstrap PATH   Demo: DNA exportieren, dann importieren\n"
              << "  --eternal               Ewiger Loop mit DNA-Merge\n"
              << "  --eternal-loops N       Anzahl Loops (Default 10, 0 = Default)\n";
}

bool parse_int(const std::string &s, int &out) {
    try { out = std::stoi(s); return true; } catch (...) { return false; }
}

bool parse_float(const std::string &s, float &out) {
    try { out = std::stof(s); return true; } catch (...) { return false; }
}
}

int main(int argc, char **argv) {
    std::string input;
    std::string output;
    LivingOptions opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto require = [&](const char *name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Fehlender Wert fuer " << name << "\n";
                std::exit(1);
            }
            return argv[++i];
        };
        if (arg == "--input") {
            input = require("--input");
        } else if (arg == "--output") {
            output = require("--output");
        } else if (arg == "--steps") {
            parse_int(require("--steps"), opt.steps);
        } else if (arg == "--agents") {
            parse_int(require("--agents"), opt.agents);
        } else if (arg == "--seed") {
            parse_int(require("--seed"), opt.seed);
        } else if (arg == "--blur-sigma") {
            parse_float(require("--blur-sigma"), opt.blur_sigma);
        } else if (arg == "--unsharp-sigma") {
            parse_float(require("--unsharp-sigma"), opt.unsharp_sigma);
        } else if (arg == "--unsharp-amount") {
            parse_float(require("--unsharp-amount"), opt.unsharp_amount);
        } else if (arg == "--unsharp-threshold") {
            parse_float(require("--unsharp-threshold"), opt.unsharp_threshold);
        } else if (arg == "--mutate") {
            parse_float(require("--mutate"), opt.mutate_strength);
        } else if (arg == "--mycel") {
            parse_float(require("--mycel"), opt.mycel_strength);
        } else if (arg == "--edge-gain") {
            parse_float(require("--edge-gain"), opt.edge_gain);
        } else if (arg == "--blur-threshold") {
            parse_float(require("--blur-threshold"), opt.blur_threshold);
        } else if (arg == "--blur-gain") {
            parse_float(require("--blur-gain"), opt.blur_gain);
        } else if (arg == "--size") {
            parse_int(require("--size"), opt.size);
        } else if (arg == "--loop-damp-start") {
            parse_int(require("--loop-damp-start"), opt.loop_damp_start);
        } else if (arg == "--loop-damp") {
            parse_float(require("--loop-damp"), opt.loop_damp);
        } else if (arg == "--commands") {
            opt.command_file = require("--commands");
        } else if (arg == "--dna-import") {
            opt.dna_import = require("--dna-import");
        } else if (arg == "--dna-export") {
            opt.dna_export = require("--dna-export");
        } else if (arg == "--dna-style") {
            opt.dna_style = require("--dna-style");
        } else if (arg == "--demo-bootstrap") {
            opt.dna_bootstrap = require("--demo-bootstrap");
        } else if (arg == "--eternal") {
            opt.eternal = true;
        } else if (arg == "--eternal-loops") {
            parse_int(require("--eternal-loops"), opt.eternal_loops);
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unbekanntes Argument: " << arg << "\n";
            print_usage();
            return 1;
        }
    }

    if (input.empty() || output.empty()) {
        print_usage();
        return 1;
    }

    std::string error;
    int rc = run_living_pipeline(input, output, opt, error);
    if (rc != 0) {
        std::cerr << "Fehler: " << error << "\n";
        return rc;
    }
    std::cout << "OK: " << output << "\n";
    return 0;
}
