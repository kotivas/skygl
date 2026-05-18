#pragma once

namespace Noise {

    float Perlin(float x, float y);
    // Lacunarity specifies the frequency multiplier between successive octaves (default to 2.0).
    // Persistence is the loss of amplitude between successive octaves (usually 1/lacunarity)
    float PerlinFBM(size_t octaves, float x, float y, float freq = 1.0, float amp = 1.0);

} // namespace Noise
