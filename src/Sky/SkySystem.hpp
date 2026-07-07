#pragma once
#include "Atmosphere/AtmosphereModel.hpp"
#include "Clouds/CloudsModel.hpp"
#include "Weather.hpp"
#include "Camera.hpp"
#include "Sun.hpp"

#include <random>
#include <algorithm>
#include <glm/ext/quaternion_common.hpp>

namespace Sky {

    struct SkyDebugInfo {
        float dayTime;
        glm::vec3 sunDirection;
        float windSpeed;
        float blendFactor;
        float transitionDuration;
        bool isTransitioning;

        GLuint currWeatherMapHandle;
        GLuint nextWeatherMapHandle;
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

    struct WeatherTransition {
        WeatherParameters from{};
        WeatherParameters to{};
        float duration = 1.0f;
        float t = 0.0f;
        bool running = false;

        void start(const WeatherParameters& a, const WeatherParameters& b, float dur) {
            from = a;
            to = b;
            duration = dur;
            t = 0.0f;
            running = true;
        }

        void update(float dt) {
            if (!running) return;
            t = std::min(t + dt / duration, 1.0f);
            if (t >= 1.0f) running = false;
        }

        [[nodiscard]] WeatherParameters current() const {
            float s = blend();
            return {
                std::lerp(from.cirrusDensity, to.cirrusDensity, s),
                std::lerp(from.altoDensity, to.altoDensity, s),
                // std::lerp(from.windSpeed, to.windSpeed, s),
                to.windSpeed,
                s,
            };
        }

    private:
        [[nodiscard]] float blend() const {
            // return ease(t);
            return t;
        }
    };

    class SkySystem {
    public:
        SkySystem();

        void initialize(const Clouds::CloudsParameters& clouds_params,
            const Atm::AtmosphereParameters& atmosphere_params,
            const SkyShaders& shaders,
            const uint8_t* highCloudsMap);

        void setWeather(const WeatherType& preset, float transitionTime);
        void setShaders(const SkyShaders& shaders);
        void setCloudsParameters(const Clouds::CloudsParameters& params);
        void setTime(float time);
        void addTime(float time);

        float getTime() const;

        void recomputeAtmosphere(const Atm::AtmosphereParameters& params);

        [[nodiscard]] SkyDebugInfo getDebugInfo() const;

        // renders entire sky to the given framebuffer
        void render(const Camera& camera, uint16_t framebuffer, float gamma, float exposure);
        void update(double dt);

        ~SkySystem();


    private:
        void atmospherePass(const Camera& camera);
        void cloudsPass(const Camera& camera);
        WeatherType pickNextWeather() const;

        Clouds::CloudsModel _cloudsModel;
        Atm::AtmosphereModel _atmosphereModel;
        Sun _sun;
        double _dayTime;

        mutable std::mt19937 _rng;

        Gl::UniformBuffer _weatherUBO;

        WeatherType _currentWeather;
        WeatherParameters _weatherParameters;
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
