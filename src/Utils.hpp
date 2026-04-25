#pragma once
#include <string>

#define PTR_SAFE_DELETE(x) \
    if (x) {               \
        delete (x);        \
        (x) = nullptr;     \
    }

namespace Utils {
    std::string ReadFromFile(const std::string& path);
    float remap(float value, float inMin, float inMax, float outMin, float outMax);
} // namespace Utils
