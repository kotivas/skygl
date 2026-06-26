#pragma once

namespace Sky {
    enum class WeatherMapPreset : uint8_t { ClearSky, ScatteredClouds, BrokenClouds, Overcast, Storm };

    struct WeatherParameters {
        float cirrusDensity; // Density of cirrus clouds
        float altoDensity;   // Density of alto clouds
        float windSpeed;
        float weatherMapBlend;
    };
} // namespace Sky
