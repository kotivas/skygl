#include "Weather.hpp"
#include "Constants.hpp"
#include "../Noise.hpp"

namespace Sky {
    namespace WeatherMapGenerator {

        std::vector<float> GenClearSky(float seed) {
            std::vector<float> weather;
            weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

            for (float& f : weather) f = 0.0f; // it's clear sky

            return weather;
        }

        std::vector<float> GenScatteredClouds(float seed) {
            std::vector<float> weather;
            weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

            for (int x = 0; x < Clouds::WEATHER_MAP_SIZE; x++) {
                for (int y = 0; y < Clouds::WEATHER_MAP_SIZE; y++) {
                    const float fx = (float)x / (float)Clouds::WEATHER_MAP_SIZE;
                    const float fy = (float)y / (float)Clouds::WEATHER_MAP_SIZE;
                    const int coord = (y * Clouds::WEATHER_MAP_SIZE + x) * 3;

                    float coverage_perlin = Noise::PerlinFBM(3, fx, fy, 24, 1, (unsigned)seed) * 0.5f + 0.5f;

                    weather[coord + 0] = glm::smoothstep(0.45f, 0.8f, coverage_perlin) * 0.7f; // coverage
                    weather[coord + 1] = 0.0f;                                                 // precipitation
                    weather[coord + 2] = 0.3f;                                                 // type
                }
            }

            return weather;
        }

        std::vector<float> GenBrokenClouds(float seed) {
            std::vector<float> weather;
            weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

            for (int x = 0; x < Clouds::WEATHER_MAP_SIZE; x++) {
                for (int y = 0; y < Clouds::WEATHER_MAP_SIZE; y++) {
                    const float fx = (float)x / (float)Clouds::WEATHER_MAP_SIZE;
                    const float fy = (float)y / (float)Clouds::WEATHER_MAP_SIZE;
                    const int coord = (y * Clouds::WEATHER_MAP_SIZE + x) * 3;

                    float coverage_worley = 1.0f - Noise::WorleyF1(fx * 32, fy * 32, 32, seed);
                    float coverage_perlin = Noise::PerlinFBM(3, fx, fy, 16, (unsigned)seed) * 0.5f + 0.5f;
                    float coverage = coverage_perlin * glm::smoothstep(0.0f, 0.9f, coverage_worley);

                    float cloudtype = glm::mix(0.0f, 0.6f, glm::smoothstep(0.1f, 0.4f, coverage)) * coverage_worley;

                    weather[coord + 0] = coverage;                         // coverage
                    weather[coord + 1] = 0.0f;                             // precipitation
                    weather[coord + 2] = coverage < 0.1 ? 0.0 : cloudtype; // type
                }
            }

            return weather;
        }

        std::vector<float> GenOvercast(float seed) {
            std::vector<float> weather;
            weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

            for (int x = 0; x < Clouds::WEATHER_MAP_SIZE; x++) {
                for (int y = 0; y < Clouds::WEATHER_MAP_SIZE; y++) {
                    const float fx = (float)x / (float)Clouds::WEATHER_MAP_SIZE;
                    const float fy = (float)y / (float)Clouds::WEATHER_MAP_SIZE;
                    const int coord = (y * Clouds::WEATHER_MAP_SIZE + x) * 3;

                    weather[coord + 0] = Noise::PerlinFBM(3, fx, fy, 16, 1, (unsigned)seed) * 0.5f + 0.5f; // coverage
                    weather[coord + 1] = 0.3f;                                                             // precipitation
                    weather[coord + 2] = 0.8f;                                                             // type
                }
            }

            return weather;
        }

        std::vector<float> GenStorm(float seed) {
            std::vector<float> weather;
            weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

            for (int x = 0; x < Clouds::WEATHER_MAP_SIZE; x++) {
                for (int y = 0; y < Clouds::WEATHER_MAP_SIZE; y++) {
                    const float fx = (float)x / (float)Clouds::WEATHER_MAP_SIZE;
                    const float fy = (float)y / (float)Clouds::WEATHER_MAP_SIZE;
                    const int coord = (y * Clouds::WEATHER_MAP_SIZE + x) * 3;

                    weather[coord + 0] = Noise::PerlinFBM(3, fx, fy, 16, 1, (unsigned)seed) * 0.5f + 0.5f; // coverage
                    weather[coord + 1] = Noise::PerlinFBM(2, fx, fy, 8, 1, (unsigned)seed) * 0.5f + 0.5f;  // precipitation
                    weather[coord + 2] = 1.0f;                                                             // type
                }
            }

            return weather;
        }
    } // namespace WeatherMapGenerator
} // namespace Sky
