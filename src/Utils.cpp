#include "Utils.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace Utils {
    std::string ReadFromFile(const std::string& path) {
        std::string content;
        std::ifstream fileStream(path, std::ios::in);
        if (!fileStream.is_open()) {
            std::cout << "Couldnt open file " << path << std::endl;
            return "";
        }
        std::string line;
        while (!fileStream.eof()) {
            std::getline(fileStream, line);
            content.append(line + "\n");
        }
        fileStream.close();
        return content;
    }

    /** Remaps value from [inMin, inMax] to [outMin, outMax] */
    float remap(float value, float inMin, float inMax, float outMin, float outMax) {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
    }

} // namespace Utils
