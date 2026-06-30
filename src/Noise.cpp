#include "Noise.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/trigonometric.hpp>

namespace Noise {

    glm::vec2 hash22(glm::vec2 p, float seed = 0.f) {
        p += seed;
        p = glm::vec2(glm::dot(p, glm::vec2(127.1f, 311.7f)), glm::dot(p, glm::vec2(269.5f, 183.3f)));
        return glm::fract(glm::sin(p) * 43758.5453123f);
    }

    // cubic interpolation
    float cubicLerp(float a0, float a1, float w) {
        return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
    }

    // hash func converting int x, y into the float x, y between 0 and 2π
    glm::vec2 randomGradient(int ix, int iy, unsigned seed = 0u) {
        const unsigned w = 8 * sizeof(unsigned);
        const unsigned s = w / 2;
        unsigned a = ix, b = iy;

        a ^= seed * 2654435761u; // prime Knuth multiplicative hash
        b ^= seed * 2246822519u;

        a *= 3284157443;
        b ^= a << s | a >> (w - s);
        b *= 1911520717;
        a ^= b << s | b >> (w - s);
        a *= 2048419325;

        const float random = a * (3.14159265f / ~(~0u >> 1));

        return glm::vec2(glm::sin(random), glm::cos(random));
    }

    float dotGridGradient(int ix, int iy, float x, float y, unsigned seed = 0u) {
        glm::vec2 gradient = randomGradient(ix, iy, seed);

        // distance vector
        float dx = x - float(ix);
        float dy = y - float(iy);

        // return dot product
        return (dx * gradient.x + dy * gradient.y);
    }

    float Perlin(float x, float y, unsigned seed) {
        // grid cell corner coords
        const int x0 = static_cast<int>(x);
        const int y0 = static_cast<int>(y);
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        // interpolation weights
        const float sx = x - static_cast<float>(x0);
        const float sy = y - static_cast<float>(y0);

        // interpolate top two corners
        float n0 = dotGridGradient(x0, y0, x, y, seed);
        float n1 = dotGridGradient(x1, y0, x, y, seed);
        const float ix0 = cubicLerp(n0, n1, sx);

        // interpolate bottom two corners
        n0 = dotGridGradient(x0, y1, x, y, seed);
        n1 = dotGridGradient(x1, y1, x, y, seed);
        const float ix1 = cubicLerp(n0, n1, sx);

        return cubicLerp(ix0, ix1, sy);
    }

    float PerlinFBM(size_t octaves, float x, float y, float freq, float amp, unsigned seed) {
        float result = 0.0;
        float denom = 0.0;

        const float gain = 0.5f;
        const float lacunarity = 2.0f;

        for (int i = 0; i < octaves; i++) {
            result += amp * Perlin(x * freq, y * freq, seed);
            denom += amp;

            amp *= gain;
            freq *= lacunarity;
        }
        return result / denom;
    }

    float WorleyF1(float x, float y, float period, float seed) {
        glm::vec2 uv = glm::vec2(x, y);
        glm::vec2 cell = glm::floor(uv);
        glm::vec2 fr = glm::fract(uv);

        float minDist = 1e9f;

        for (int j = -1; j <= 1; j++) {
            for (int i = -1; i <= 1; i++) {
                glm::vec2 neighbor = glm::vec2(float(i), float(j));

                // тайловая ячейка
                glm::vec2 tiledCell;
                tiledCell.x = std::fmod(cell.x + neighbor.x + period, period);
                tiledCell.y = std::fmod(cell.y + neighbor.y + period, period);

                glm::vec2 point = hash22(tiledCell, seed) + neighbor;
                float dist = glm::length(point - fr);
                minDist = glm::min(minDist, dist);
            }
        }
        return minDist;
    }

} // namespace Noise
