#include "CloudsModel.hpp"

#include "Noise.hpp"
#include "Utils.hpp"
#include "Sky/Constants.hpp"

#include <iostream>

namespace Sky::Clouds {

    CloudsModel::CloudsModel() : _baseNoise(nullptr), _detailNoise(nullptr), _weatherMap(nullptr), _highCloudsMap(nullptr) {}

    Gl::Texture* CloudsModel::getWeatherMap() const {
        return _weatherMap;
    }
    void CloudsModel::generateWeatherMap(float freq, float edge0, float edge1) const {
        std::vector<float> weather;
        weather.resize(WEATHER_MAP_SIZE * WEATHER_MAP_SIZE * 3);

        for (int x = 0; x < WEATHER_MAP_SIZE; x++) {
            for (int y = 0; y < WEATHER_MAP_SIZE; y++) {
                const float fx = (float)x / (float)WEATHER_MAP_SIZE;
                const float fy = (float)y / (float)WEATHER_MAP_SIZE;
                const int coord = (y * WEATHER_MAP_SIZE + x) * 3;

                const float c = Noise::PerlinWarp(fx, fy, 0, freq);
                const float coverage = glm::smoothstep(edge0, edge1, c);

                // const float prec = 0.5f + Noise::PerlinFBM(3, fx, fy, w_freq, 1.0f) * 0.5f;
                const float type = 0.5f + Noise::Perlin(fx * 8.0f, fy * 8.0f) * 0.5f;

                weather[coord + 0] = coverage; // coverage
                weather[coord + 1] = 0;        // precipitation
                weather[coord + 2] = type;     // type
            }
        }

        // the size of internal weather map should be generally the same as WEATHER_MAP_SIZE
        _weatherMap->setData(weather.data(), _weatherMap->getWidth(), _weatherMap->getHeight());
    }

    void CloudsModel::bind(Gl::Shader* shader) const {
        shader->use();

        _paramsUBO.bind(1);
        shader->setUniform1i("baseNoise", 4);
        _baseNoise->bind(4);
        shader->setUniform1i("detailNoise", 5);
        _detailNoise->bind(5);
        shader->setUniform1i("weatherMap", 6);
        _weatherMap->bind(6);
        shader->setUniform1i("highCloudsMap", 7);
        _highCloudsMap->bind(7);
    }

    void CloudsModel::updateParameters(const CloudsParameters& params) {
        _params = params;

        _params.maxDistance /= LenghtUnitInMeters;
        _params.sigmaS *= LenghtUnitInMeters;
        _params.sigmaA *= LenghtUnitInMeters;

        _params.cloudLayerBottom /= LenghtUnitInMeters;
        _params.cloudLayerThickness /= LenghtUnitInMeters;
        _params.highCloudsHeight /= LenghtUnitInMeters;

        _params.highCloudsScale /= LenghtUnitInMeters;
        _params.weatherMapScale /= LenghtUnitInMeters;
        _params.baseNoiseScale /= LenghtUnitInMeters;
        _params.detailNoiseScale /= LenghtUnitInMeters;

        _paramsUBO.setData(&_params);
    }

    void CloudsModel::setHighCloudsMap(const uint8_t* data) const {
        _highCloudsMap->setData(data, _highCloudsMap->getWidth(), _highCloudsMap->getHeight());
    }

    void CloudsModel::initialize(const CloudsParameters& params) {
        _paramsUBO.create(sizeof(CloudsParameters));

        updateParameters(params);

        /* Weather Map
         * .R - coverage
         * .G - precipitation
         * .B - cloud type
         */
        _weatherMap = new Gl::Texture();
        _weatherMap->create(WEATHER_MAP_SIZE, WEATHER_MAP_SIZE, GL_RGB, GL_RGB16F, GL_FLOAT);
        _weatherMap->setWrapMode(GL_REPEAT);
        _weatherMap->setMinFilter(GL_LINEAR);
        _weatherMap->setMagFilter(GL_LINEAR);

        /* High Altitude Map
         * .R - cirrus
         * .G - alto
         */
        _highCloudsMap = new Gl::Texture();
        _highCloudsMap->create(HIGH_CLOUDS_MAP_SIZE, HIGH_CLOUDS_MAP_SIZE, GL_RG, GL_RG8, GL_UNSIGNED_BYTE);
        _highCloudsMap->setWrapMode(GL_REPEAT);
        _highCloudsMap->setMinFilter(GL_LINEAR);
        _highCloudsMap->setMagFilter(GL_LINEAR);

        /* ==== Base Noise
         * .R - Perlin-Worley
         * .GBA - Worley noise at increasing frequencies
         */
        _baseNoise = new Gl::Texture3D();
        _baseNoise->create(BASE_NOISE_SIZE, BASE_NOISE_SIZE, BASE_NOISE_SIZE, GL_RGBA, GL_RGBA16F, GL_FLOAT, false);
        _baseNoise->setWrapMode(GL_REPEAT);
        _baseNoise->setMinFilter(GL_LINEAR);
        _baseNoise->setMagFilter(GL_LINEAR);

        /* ==== Detail Noise
         * .RGB - Worley noise at increasing frequencies
         * (.a used only for compute shader specifications)
         */
        _detailNoise = new Gl::Texture3D();
        _detailNoise->create(DETAIL_NOISE_SIZE, DETAIL_NOISE_SIZE, DETAIL_NOISE_SIZE, GL_RGBA, GL_RGBA16F, GL_FLOAT, false);
        _detailNoise->setWrapMode(GL_REPEAT);
        _detailNoise->setMinFilter(GL_LINEAR);
        _detailNoise->setMagFilter(GL_LINEAR);
    }

    void CloudsModel::generateBaseNoise(const Gl::ComputeShader* shader) const {
        if (!shader) return;

        shader->use();

        _baseNoise->bindImage(0, GL_WRITE_ONLY);

        constexpr int groupSize = BASE_NOISE_SIZE / 8; // local size = 8
        shader->dispatch(groupSize, groupSize, groupSize);

        shader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void CloudsModel::generateDetailNoise(const Gl::ComputeShader* shader) const {
        if (!shader) return;

        shader->use();

        _detailNoise->bindImage(1, GL_WRITE_ONLY);

        constexpr int groupSize = DETAIL_NOISE_SIZE / 8; // local size = 8
        shader->dispatch(groupSize, groupSize, groupSize);

        shader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    CloudsModel::~CloudsModel() {
        PTR_SAFE_DELETE(_weatherMap);
        PTR_SAFE_DELETE(_baseNoise);
        PTR_SAFE_DELETE(_detailNoise);
        PTR_SAFE_DELETE(_highCloudsMap);
    }

} // namespace Sky::Clouds
