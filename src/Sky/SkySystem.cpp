#include "SkySystem.hpp"

#include "Input.hpp"
#include "Noise.hpp"
#include "Gl/GlUtils.hpp"

/* todo
 * RenderContextInfo struct
 * smooth windspeed transition
 * rescale detail and base cloud noises
 */

namespace Sky {

    SkySystem::SkySystem() :
        _atmosphereShader(nullptr), _cloudsShader(nullptr), _composeShader(nullptr), _atmosphereFBO(0), _atmosphereColor(0), _cloudsFBO(0), _cloudsColor(0),
        _quadVao(0), _quadVbo(0), _dayTime(43200) {}

    void SkySystem::initialize(const Clouds::CloudsParameters& clouds_params,
        const Atm::AtmosphereParameters& atmosphere_params,
        const SkyShaders& shaders,
        const uint8_t* highCloudsMap) {

        assert(highCloudsMap);

        setShaders(shaders);

        // --- framebuffers

        Gl::CreateQuadVO(_quadVao, _quadVbo);
        Gl::CreateFrameBuffer(_atmosphereFBO, _atmosphereColor, GL_RGBA32F, GL_RGBA, ATMOSPHERE_FRAME_WIDTH, ATMOSPHERE_FRAME_HEIGHT);
        Gl::CreateFrameBuffer(_cloudsFBO, _cloudsColor, GL_RGBA16F, GL_RGBA, CLOUD_FRAME_WIDTH, CLOUD_FRAME_HEIGHT);

        // --- Atmosphere ---
        _atmosphereModel.initialize(atmosphere_params);

        // --- Clouds ---

        _cloudsModel.initialize(clouds_params);
        _cloudsModel.generateBaseNoise(shaders.baseNoiseShader);
        _cloudsModel.generateDetailNoise(shaders.detailNoiseShader);
        _cloudsModel.setHighCloudsMap(highCloudsMap);
        delete highCloudsMap;

        // --- Weather UBO ---
        _weatherUBO.create(sizeof(WeatherParameters));
        _weatherParameters.altoDensity = 0.05f;
        _weatherParameters.cirrusDensity = 0.05f;
        _weatherParameters.windSpeed = 2 / LenghtUnitInMeters;
        _weatherUBO.setData(&_weatherParameters);

        setWeather(WeatherMapPreset::ScatteredClouds, 1);
    }

    void SkySystem::atmospherePass(const Camera& camera) {
        assert(_atmosphereShader);
        if (!_atmosphereShader->isValid()) return;

        _atmosphereShader->use();

        _atmosphereShader->setUniform1f("uTime", _dayTime);
        const auto viewNoTrans = glm::mat4(glm::mat3(camera.viewMatrix));
        _atmosphereShader->setUniformMat4fv("uView", viewNoTrans);
        _atmosphereShader->setUniformMat4fv("uProjection", camera.projectionMatrix);
        _atmosphereShader->setUniform3f("uCameraPos", camera.position);

        _atmosphereShader->setUniform3f("uSunDir", _sun.direction);
        _atmosphereShader->setUniform2f("uSunSize", glm::vec2(tan(SUN_ANGULAR_RADIUS), cos(SUN_ANGULAR_RADIUS)));

        _atmosphereModel.bind_uniform(_atmosphereShader);

        glViewport(0, 0, ATMOSPHERE_FRAME_WIDTH, ATMOSPHERE_FRAME_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, _atmosphereFBO);

        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(_quadVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void SkySystem::cloudsPass(const Camera& camera) {
        assert(_cloudsShader);
        if (!_cloudsShader->isValid()) return;

        _cloudsShader->use();
        _cloudsShader->setUniform1f("uTime", _dayTime);
        _cloudsShader->setUniformMat4fv("uView", camera.viewMatrix);
        _cloudsShader->setUniformMat4fv("uProjection", camera.projectionMatrix);
        _cloudsShader->setUniform3f("uCameraPos", camera.position);
        _cloudsShader->setUniform3f("uSunDir", _sun.direction);

        _cloudsModel.bind(_cloudsShader);
        _atmosphereModel.bind_uniform(_cloudsShader);
        _weatherUBO.bind(2);

        glViewport(0, 0, CLOUD_FRAME_WIDTH, CLOUD_FRAME_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, _cloudsFBO);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(_quadVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void SkySystem::render(const Camera& camera, uint16_t framebuffer, float gamma, float exposure) {

        atmospherePass(camera);
        cloudsPass(camera);

        // collect all data into the RenderInfo and use it for info, instead of scene

        // ---

        assert(_composeShader);
        if (!_composeShader->isValid()) return;

        _composeShader->use();

        _composeShader->setUniform1f("uGamma", gamma);
        _composeShader->setUniform1f("uExposure", exposure);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _atmosphereColor);
        _composeShader->setUniform1i("skyTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _cloudsColor);
        _composeShader->setUniform1i("cloudsTexture", 1);

        glViewport(0, 0, Input::GetWindowWidth(), Input::GetWindowHeight());

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(_quadVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }

    void SkySystem::setShaders(const SkyShaders& shaders) {
        _atmosphereModel.clear2DShader = shaders.clear2DShader;
        _atmosphereModel.clear3DShader = shaders.clear3DShader;
        _atmosphereModel.transmittanceShader = shaders.transmittanceShader;
        _atmosphereModel.directIrradianceShader = shaders.directIrradianceShader;
        _atmosphereModel.indirectIrradianceShader = shaders.indirectIrradianceShader;
        _atmosphereModel.multipleScatteringShader = shaders.multipleScatteringShader;
        _atmosphereModel.scatteringDensityShader = shaders.scatteringDensityShader;
        _atmosphereModel.singleScatteringShader = shaders.singleScatteringShader;

        _atmosphereShader = shaders.atmosphereShader;
        _cloudsShader = shaders.cloudsShader;
        _composeShader = shaders.composeShader;
    }

    std::vector<float> SkySystem::generateWeatherMap(WeatherMapPreset preset) {
        std::vector<float> weather;
        weather.resize(Clouds::WEATHER_MAP_SIZE * Clouds::WEATHER_MAP_SIZE * 3);

        for (int x = 0; x < Clouds::WEATHER_MAP_SIZE; x++) {
            for (int y = 0; y < Clouds::WEATHER_MAP_SIZE; y++) {
                const float fx = (float)x / (float)Clouds::WEATHER_MAP_SIZE;
                const float fy = (float)y / (float)Clouds::WEATHER_MAP_SIZE;
                const int coord = (y * Clouds::WEATHER_MAP_SIZE + x) * 3;

                float coverage = 0;
                float precipitation = 0;
                float cloudtype = 0;

                switch (preset) {

                case WeatherMapPreset::ClearSky: {
                    break;
                }
                case WeatherMapPreset::ScatteredClouds: {
                    float coverage_worley = 1.0f - Noise::WorleyF1(fx * 24, fy * 24, 24);
                    float coverage_perlin = Noise::PerlinFBM(3, fx, fy, 16) * 0.5f + 0.5f;

                    coverage = coverage_perlin * glm::smoothstep(0.0f, 0.9f, coverage_worley);
                    cloudtype = glm::mix(0.0f, 0.6f, glm::smoothstep(0.1f, 0.9f, coverage));
                    cloudtype = coverage < 0.1 ? 0.0 : cloudtype;
                    precipitation = 0;
                    break;
                }
                case WeatherMapPreset::BrokenClouds: {
                    break;
                }
                case WeatherMapPreset::Overcast: {
                    coverage = Noise::PerlinFBM(3, fx, fy, 16) * 0.5f + 0.5f;
                    precipitation = 0.3;
                    cloudtype = 0.8;
                    break;
                }
                case WeatherMapPreset::Storm: {
                    break;
                }
                }

                weather[coord + 0] = coverage;      // coverage
                weather[coord + 1] = precipitation; // precipitation
                weather[coord + 2] = cloudtype;     // type
            }
        }

        return weather;
    }

    void SkySystem::setTime(float time) {
        _dayTime = time;
    }

    void SkySystem::addTime(float time) {
        _dayTime += time;
    }

    float SkySystem::getTime() const {
        return _dayTime;
    }

    Gl::Texture* SkySystem::getCurrentWeatherMap() const {
        return _cloudsModel.getCurrentWeatherMap();
    }

    Gl::Texture* SkySystem::getNextWeatherMap() const {
        return _cloudsModel.getNextWeatherMap();
    }

    void SkySystem::setCloudsParameters(const Clouds::CloudsParameters& params) {
        _cloudsModel.updateParameters(params);
    }

    void SkySystem::recomputeAtmosphere(const Atm::AtmosphereParameters& params) {
        _atmosphereModel.initialize(params);
    }

    void SkySystem::setWeather(const WeatherMapPreset& params, float transitionTime) {
        _cloudsModel.swapWeatherMap();

        std::vector<float> weather = generateWeatherMap(params);
        _cloudsModel.setNextWeatherMap(weather.data());

        _weatherTransition.duration = transitionTime;
        _weatherTransition.t = 0.0f;
        _weatherTransition.running = true;
    }

    SkyDebugInfo SkySystem::getDebugInfo() const {
        SkyDebugInfo debugInfo;
        debugInfo.dayTime = _dayTime;
        debugInfo.sunDirection = _sun.direction;
        debugInfo.blendFactor = _weatherTransition.blend();
        debugInfo.transitionDuration = _weatherTransition.duration;
        debugInfo.isTransitioning = _weatherTransition.running;

        return debugInfo;
    }

    void SkySystem::update(double dt) {
        _dayTime += dt * TIME_MULTIPLIER;

        _weatherTransition.update(dt);
        if (_weatherTransition.running) {
            _weatherParameters.weatherMapBlend = _weatherTransition.blend();
            _weatherUBO.setData(&_weatherParameters);
        }

        _sun.update(_dayTime, 80); // 157
    }

    SkySystem::~SkySystem() {
        glDeleteVertexArrays(1, &_quadVao);
        glDeleteBuffers(1, &_quadVbo);
    }


} // namespace Sky
