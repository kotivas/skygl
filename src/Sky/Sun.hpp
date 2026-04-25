#pragma once
#include "Constants.hpp"
#include <glm/glm.hpp>

namespace Sky {
    struct Sun {
        glm::vec3 direction;

        void update(double timeofday, int dayofyear) {
            float lat = glm::radians(52.f); // LAT
            float epsilon = glm::radians(23.44f);
            float dayFrac = fmod(timeofday, DAY_LENGHT) / DAY_LENGHT; // 0..1

            // δ = ε * sin(2π (d - d0)/365), d0 ≈ 80 — vernal equinox
            float delta = epsilon * sin(2.0f * glm::pi<float>() * (dayofyear - 80) / 365.0f);

            // 0 = midday, -π = dawn, +π = sunset
            float H = (dayFrac - 0.5f) * glm::two_pi<float>();

            float sinAlt = sin(lat) * sin(delta) + cos(lat) * cos(delta) * cos(H);
            float cosAlt = sqrt(glm::max(0.0f, 1.0f - sinAlt * sinAlt));

            direction.x = cosAlt * sin(H);
            direction.y = sinAlt;
            direction.z = cosAlt * cos(H);
        }
    };

} // namespace Sky
