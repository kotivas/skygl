#pragma once
#include <vector>
#include <unordered_map>

namespace Sky {
    enum class WeatherType : uint8_t { Clear, MostlyClear, PartlyCloudy, MostlyCloudy, Overcast, Storm };


    static const std::unordered_map<WeatherType, std::vector<std::pair<WeatherType, float>>> WeatherGraph = {
        { WeatherType::Clear, {
            { WeatherType::MostlyClear,  0.25f },
            { WeatherType::PartlyCloudy, 0.1f }
        }},
        { WeatherType::MostlyClear, {
            { WeatherType::Clear,        0.20f },
            { WeatherType::PartlyCloudy, 0.15f },
            { WeatherType::MostlyCloudy, 0.05f }
        }},
        { WeatherType::PartlyCloudy, {
            { WeatherType::Clear,        0.05f },
            { WeatherType::MostlyClear,  0.25f },
            { WeatherType::MostlyCloudy, 0.15f },
            { WeatherType::Overcast,     0.05f }
        }},
        { WeatherType::MostlyCloudy, {
            { WeatherType::MostlyClear,  0.05f },
            { WeatherType::PartlyCloudy, 0.15f },
            { WeatherType::Overcast,     0.25f },
            { WeatherType::Storm,        0.05f }
        }},
        { WeatherType::Overcast, {
            { WeatherType::PartlyCloudy, 0.05f },
            { WeatherType::MostlyCloudy, 0.20f },
            { WeatherType::Storm,        0.15f }
        }},
        { WeatherType::Storm, {
            { WeatherType::MostlyCloudy, 0.05f },
            { WeatherType::Overcast,     0.35f },
            { WeatherType::Storm,        0.60f }
        }}
    };

    struct WeatherParameters {
        float cirrusDensity; // Density of cirrus clouds
        float altoDensity;   // Density of alto clouds
        float windSpeed;
        float weatherMapBlend;
    };

    namespace WeatherMapGenerator {
        std::vector<float> GenClear
        (float seed = 0.0f);
        std::vector<float> GenMostlyClear(float seed = 0.0f);
        std::vector<float> GenPartlyCloudy(float seed = 0.0f);
        std::vector<float> GenMostlyCloudy(float seed = 0.0f);
        std::vector<float> GenOvercast(float seed = 0.0f);
        std::vector<float> GenStorm(float seed = 0.0f);
    }; // namespace WeatherMapGenerator
} // namespace Sky
