#pragma once
#include "Atmosphere/AtmosphereModel.hpp"
#include "Clouds/CloudsModel.hpp"
#include "Weather.hpp"
#include "Camera.hpp"
#include "Sun.hpp"

#include <algorithm>
#include <glm/ext/quaternion_common.hpp>

namespace Sky {

    struct SkyDebugInfo {
        float dayTime;
        glm::vec3 sunDirection;

        float cloudsCoverage;
        float windSpeed;
        float precipitation;
        float transitionElapsed;
        float transitionDuration;
        bool isTransitioning;
    };

    struct WeatherTransition {
        WeatherParameters from;
        WeatherParameters to;
        WeatherParameters value;
        float duration;
        float elapsed;
        bool running;

        void update(double dt) {
            if (!running) return;

            elapsed += dt;

            float t = std::clamp(elapsed / duration, 0.0f, 1.0f);
            // t = easing(t)

            value.cloudsCoverage = std::lerp(from.cloudsCoverage, to.cloudsCoverage, t);
            value.precipitation = std::lerp(from.precipitation, to.precipitation, t);
            value.cirrusDensity = std::lerp(from.cirrusDensity, to.cirrusDensity, t);
            value.altoDensity = std::lerp(from.altoDensity, to.altoDensity, t);

            if (elapsed >= duration) running = false;
        }
    };

    struct SkyShaders {
        Gl::ComputeShader* clear2DShader;
        Gl::ComputeShader* clear3DShader;
        Gl::ComputeShader* transmittanceShader;
        Gl::ComputeShader* directIrradianceShader;
        Gl::ComputeShader* indirectIrradianceShader;
        Gl::ComputeShader* multipleScatteringShader;
        Gl::ComputeShader* scatteringDensityShader;
        Gl::ComputeShader* singleScatteringShader;

        Gl::ComputeShader* baseNoiseShader;
        Gl::ComputeShader* detailNoiseShader;

        Gl::Shader* atmosphereShader;
        Gl::Shader* cloudsShader;
        Gl::Shader* composeShader;
    };

    class SkySystem {
    public:
        SkySystem();

        void initialize(const Clouds::CloudsParameters& clouds_params,
            const Atm::AtmosphereParameters& atmosphere_params,
            const SkyShaders& shaders,
            const uint8_t* highCloudsMap);

        void setWeather(const WeatherParameters& params, float transitionTime);
        void setShaders(const SkyShaders& shaders);
        void setAtmosphereParameters(const Atm::AtmosphereParameters& params);

        SkyDebugInfo getDebugInfo() const;

        // renders entire sky to the given framebuffer
        void render(const Camera& camera, uint16_t framebuffer, float gamma, float exposure);
        void update(double dt);

        ~SkySystem();


    private:
        void atmospherePass(const Camera& camera);
        void cloudsPass(const Camera& camera);

        Clouds::CloudsModel _cloudsModel;
        Atm::AtmosphereModel _atmosphereModel;
        Sun _sun;
        double _dayTime;


        Gl::UniformBuffer _weatherUBO;

        WeatherTransition _weatherTransition;

        Gl::Shader* _atmosphereShader;
        Gl::Shader* _cloudsShader;
        Gl::Shader* _composeShader;

        uint32_t _atmosphereFBO;
        uint32_t _atmosphereColor;
        uint32_t _cloudsFBO;
        uint32_t _cloudsColor;
        uint32_t _quadVao, _quadVbo;
    };
} // namespace Sky
