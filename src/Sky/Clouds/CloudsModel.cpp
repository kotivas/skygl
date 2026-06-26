#include "CloudsModel.hpp"

#include "Noise.hpp"
#include "Utils.hpp"
#include "Sky/Constants.hpp"

#include <iostream>

namespace Sky::Clouds {

    CloudsModel::CloudsModel() : _baseNoise(nullptr), _detailNoise(nullptr), _currentWeatherMap(nullptr), _highCloudsMap(nullptr) {}

    Gl::Texture* CloudsModel::getCurrentWeatherMap() const {
        return _currentWeatherMap;
    }

    Gl::Texture* CloudsModel::getNextWeatherMap() const {
        return _nextWeatherMap;
    }

    void CloudsModel::setNextWeatherMap(const float* next) const {
        _nextWeatherMap->setData(next, _nextWeatherMap->getWidth(), _nextWeatherMap->getHeight());
    }

    void CloudsModel::swapWeatherMap() {
        auto next = _nextWeatherMap;
        _nextWeatherMap = _currentWeatherMap;
        _currentWeatherMap = next;
    }

    void CloudsModel::bind(Gl::Shader* shader) const {
        shader->use();

        _paramsUBO.bind(1);
        shader->setUniform1i("baseNoise", 4);
        _baseNoise->bind(4);
        shader->setUniform1i("detailNoise", 5);
        _detailNoise->bind(5);
        shader->setUniform1i("currentWeatherMap", 6);
        _currentWeatherMap->bind(6);
        shader->setUniform1i("nextWeatherMap", 7);
        _nextWeatherMap->bind(7);
        shader->setUniform1i("highCloudsMap", 8);
        _highCloudsMap->bind(8);
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
        _currentWeatherMap = new Gl::Texture();
        _currentWeatherMap->create(WEATHER_MAP_SIZE, WEATHER_MAP_SIZE, GL_RGB, GL_RGB16F, GL_FLOAT);
        _currentWeatherMap->setWrapMode(GL_REPEAT);
        _currentWeatherMap->setMinFilter(GL_LINEAR);
        _currentWeatherMap->setMagFilter(GL_LINEAR);

        _nextWeatherMap = new Gl::Texture();
        _nextWeatherMap->create(WEATHER_MAP_SIZE, WEATHER_MAP_SIZE, GL_RGB, GL_RGB16F, GL_FLOAT);
        _nextWeatherMap->setWrapMode(GL_REPEAT);
        _nextWeatherMap->setMinFilter(GL_LINEAR);
        _nextWeatherMap->setMagFilter(GL_LINEAR);

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
        PTR_SAFE_DELETE(_currentWeatherMap);
        PTR_SAFE_DELETE(_baseNoise);
        PTR_SAFE_DELETE(_detailNoise);
        PTR_SAFE_DELETE(_highCloudsMap);
    }

} // namespace Sky::Clouds
