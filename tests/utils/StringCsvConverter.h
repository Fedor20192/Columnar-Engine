#pragma once

#include <fstream>
#include <string>
#include <vector>

class Converter {
public:
    static void StringsToCsv(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream ofs(filename);

        if (!ofs.is_open()) {
            throw std::runtime_error("Could not open file for writing");
        }

        for (const auto& line : lines) {
            ofs << line << std::endl;
        }
    }
};