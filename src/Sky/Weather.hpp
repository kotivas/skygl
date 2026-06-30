#pragma once
#include <vector>
#include <unordered_map>

namespace Sky {
    enum class WeatherType : uint8_t { Clear, Scattered, Broken, Overcast, Storm };


    static const std::unordered_map<WeatherType, std::vector<std::pair<WeatherType, float>>> WeatherGraph = {
        {WeatherType::Clear, {{WeatherType::Clear, 0.5f}, {WeatherType::Scattered, 1.0f}}},
        {WeatherType::Scattered, {{WeatherType::Clear, 0.8f}, {WeatherType::Broken, 1.0f}}},
        {WeatherType::Broken, {{WeatherType::Scattered, 0.7f}, {WeatherType::Overcast, 1.0f}}},
        {WeatherType::Overcast, {{WeatherType::Broken, 0.8f}, {WeatherType::Storm, 0.4f}}},
        {WeatherType::Storm, {{WeatherType::Overcast, 1.0f}}},
    };

    struct WeatherParameters {
        float cirrusDensity; // Density of cirrus clouds
        float altoDensity;   // Density of alto clouds
        float windSpeed;
        float weatherMapBlend;
    };

    namespace WeatherMapGenerator {
        std::vector<float> GenClearSky(float seed = 0.0f);
        std::vector<float> GenScatteredClouds(float seed = 0.0f);
        std::vector<float> GenBrokenClouds(float seed = 0.0f);
        std::vector<float> GenOvercast(float seed = 0.0f);
        std::vector<float> GenStorm(float seed = 0.0f);
    }; // namespace WeatherMapGenerator
} // namespace Sky
