#include "image_io.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace {
bool read_token(std::istream &is, std::string &out) {
    out.clear();
    char c = 0;
    while (is.get(c)) {
        if (c == '#') {
            std::string dummy;
            std::getline(is, dummy);
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(c);
            break;
        }
    }
    while (is.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            break;
        }
        out.push_back(c);
    }
    return !out.empty();
}
}

bool load_ppm(const std::string &path, ImageU8 &out, std::string &error) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        error = "Datei konnte nicht geoeffnet werden: " + path;
        return false;
    }
    std::string magic;
    if (!read_token(file, magic) || magic != "P6") {
        error = "Ungueltiges PPM-Format (nur P6): " + path;
        return false;
    }
    std::string wstr, hstr, maxstr;
    if (!read_token(file, wstr) || !read_token(file, hstr) || !read_token(file, maxstr)) {
        error = "PPM-Header unvollstaendig: " + path;
        return false;
    }
    int width = std::stoi(wstr);
    int height = std::stoi(hstr);
    int maxval = std::stoi(maxstr);
    if (width <= 0 || height <= 0 || maxval != 255) {
        error = "PPM-Header ungueltig (maxval muss 255 sein): " + path;
        return false;
    }
    out.width = width;
    out.height = height;
    out.rgb.resize(static_cast<size_t>(width) * height * 3);
    file.read(reinterpret_cast<char*>(out.rgb.data()), static_cast<std::streamsize>(out.rgb.size()));
    if (!file) {
        error = "PPM-Daten unvollstaendig: " + path;
        return false;
    }
    return true;
}

bool save_ppm(const std::string &path, int width, int height, const std::vector<uint8_t> &rgb, std::string &error) {
    if (width <= 0 || height <= 0) {
        error = "Ungueltige Bildgroesse fuer PPM";
        return false;
    }
    if (static_cast<int>(rgb.size()) != width * height * 3) {
        error = "RGB-Daten haben falsche Laenge";
        return false;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        error = "Datei konnte nicht geschrieben werden: " + path;
        return false;
    }
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    if (!file) {
        error = "Fehler beim Schreiben der PPM-Daten";
        return false;
    }
    return true;
}
