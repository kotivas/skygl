#pragma once

namespace Sky {

    struct WeatherParameters {
        float precipitation;
        float cloudsCoverage; // Volumetric cloud coverage percentage
        float cirrusDensity;  // Density of cirrus clouds
        float altoDensity;    // Density of alto clouds

        float windSpeed;
    };

    // --====-- PRESETS --====--
    namespace Preset {

        constexpr WeatherParameters Cloudy = {
            .precipitation = 0.0f,
            .cloudsCoverage = 1.0f,
            .cirrusDensity = 0.05f,
            .altoDensity = 0.05f,

            .windSpeed = 3.0f,
        };

        constexpr WeatherParameters Clear = {
            .precipitation = 0.0f,
            .cloudsCoverage = 0.0f,
            .cirrusDensity = 0.0f,
            .altoDensity = 0.0f,

            .windSpeed = 0.5f,
        };

        constexpr WeatherParameters Clear2 = {
            .precipitation = 0.0f,
            .cloudsCoverage = 0.0f,
            .cirrusDensity = 0.05f,
            .altoDensity = 0.05f,

            .windSpeed = 1.0f,
        };

        constexpr WeatherParameters Overcast = {
            .precipitation = 0.0f,
            .cloudsCoverage = 1.5f,
            .cirrusDensity = 0.1f,
            .altoDensity = 0.1f,

            .windSpeed = 5.0f,
        };

        constexpr WeatherParameters Storm = {
            .precipitation = 1.0f,
            .cloudsCoverage = 3.0f,
            .cirrusDensity = 0.0f,
            .altoDensity = 0.0f,

            .windSpeed = 20.0f,
        };
    } // namespace Preset
} // namespace Sky
