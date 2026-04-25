#include "CloudsModel.hpp"

#include "Utils.hpp"
#include "Sky/Constants.hpp"

#include <iostream>
#include <glm/ext/scalar_constants.hpp>

namespace Sky::Clouds {

    CloudsModel::CloudsModel() : m_params(), m_baseNoise(nullptr), m_detailNoise(nullptr), m_weatherMap(nullptr), m_highCloudsMap(nullptr) {}

    void CloudsModel::bind(Gl::Shader* shader) const {
        shader->use();

        uniformBuffer.bind(1);
        shader->setUniform1i("baseNoise", 4);
        m_baseNoise->bind(4);
        shader->setUniform1i("detailNoise", 5);
        m_detailNoise->bind(5);
        shader->setUniform1i("weatherMap", 6);
        m_weatherMap->bind(6);
        shader->setUniform1i("highCloudsMap", 7);
        m_highCloudsMap->bind(7);
    }

    void CloudsModel::updateParameters(const CloudsParameters& params) {
        m_params = params;

        // fixme if I ever implement getting internal parameters of a cloud model, i need to change this

        // scale units
        m_params.cloudLayerBottom /= LenghtUnitInMeters;
        m_params.cloudLayerThickness /= LenghtUnitInMeters;
        m_params.highCloudsHeight /= LenghtUnitInMeters;
        m_params.maxDistance /= LenghtUnitInMeters;

        m_params.highCloudsScale /= LenghtUnitInMeters;
        m_params.weatherMapScale /= LenghtUnitInMeters;
        m_params.baseNoiseScale /= LenghtUnitInMeters;
        m_params.detailNoiseScale /= LenghtUnitInMeters;

        m_params.sigmaS *= LenghtUnitInMeters;
        m_params.sigmaA *= LenghtUnitInMeters;

        std::cout << "just in case" << std::endl;

        uniformBuffer.setData(&m_params);
    }

    void CloudsModel::setWeatherMap(const uint8_t* data) const {
        m_weatherMap->setData(data, m_weatherMap->getWidth(), m_weatherMap->getHeight());
    }
    void CloudsModel::setHighCloudsMap(const uint8_t* data) const {
        m_highCloudsMap->setData(data, m_highCloudsMap->getWidth(), m_highCloudsMap->getHeight());
    }

    void CloudsModel::initialize(const CloudsParameters& params) {
        uniformBuffer.create(sizeof(CloudsParameters));

        updateParameters(params);

        /* Weather Map
         * .R - coverage
         * .G - precipitation (density)
         * .B - cloud type
         */
        m_weatherMap = new Gl::Texture();
        m_weatherMap->create(WEATHER_MAP_SIZE, WEATHER_MAP_SIZE, GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE);
        m_weatherMap->setWrapMode(GL_REPEAT);
        m_weatherMap->setMinFilter(GL_LINEAR);
        m_weatherMap->setMagFilter(GL_LINEAR);

        /* High Altitude Map
         * .R - cirrus
         * .G - alto
         */
        m_highCloudsMap = new Gl::Texture();
        m_highCloudsMap->create(HIGH_CLOUDS_MAP_SIZE, HIGH_CLOUDS_MAP_SIZE, GL_RG, GL_RG8, GL_UNSIGNED_BYTE);
        m_highCloudsMap->setWrapMode(GL_REPEAT);
        m_highCloudsMap->setMinFilter(GL_LINEAR);
        m_highCloudsMap->setMagFilter(GL_LINEAR);

        /* ==== Base Noise
         * .R - Perlin-Worley
         * .GBA - Worley noise at increasing frequencies
         */
        m_baseNoise = new Gl::Texture3D();
        m_baseNoise->create(BASE_NOISE_SIZE, BASE_NOISE_SIZE, BASE_NOISE_SIZE, GL_RGBA, GL_RGBA16F, GL_FLOAT, false);
        m_baseNoise->setWrapMode(GL_REPEAT);
        m_baseNoise->setMinFilter(GL_LINEAR);
        m_baseNoise->setMagFilter(GL_LINEAR);

        /* ==== Detail Noise
         * .RGB - Worley noise at increasing frequencies
         * (.a used only for compute shader specifications)
         */
        m_detailNoise = new Gl::Texture3D();
        m_detailNoise->create(DETAIL_NOISE_SIZE, DETAIL_NOISE_SIZE, DETAIL_NOISE_SIZE, GL_RGBA, GL_RGBA16F, GL_FLOAT, false);
        m_detailNoise->setWrapMode(GL_REPEAT);
        m_detailNoise->setMinFilter(GL_LINEAR);
        m_detailNoise->setMagFilter(GL_LINEAR);
    }

    void CloudsModel::generateBaseNoise(const Gl::ComputeShader& shader) const {
        shader.use();

        m_baseNoise->bindImage(0, GL_WRITE_ONLY);

        constexpr int groupSize = BASE_NOISE_SIZE / 8; // local size = 8
        shader.dispatch(groupSize, groupSize, groupSize);

        shader.barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void CloudsModel::generateDetailNoise(const Gl::ComputeShader& shader) const {
        shader.use();

        m_detailNoise->bindImage(1, GL_WRITE_ONLY);

        constexpr int groupSize = DETAIL_NOISE_SIZE / 8; // local size = 8
        shader.dispatch(groupSize, groupSize, groupSize);

        shader.barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    CloudsModel::~CloudsModel() {
        PTR_SAFE_DELETE(m_weatherMap);
        PTR_SAFE_DELETE(m_baseNoise);
        PTR_SAFE_DELETE(m_detailNoise);
        PTR_SAFE_DELETE(m_highCloudsMap);
    }

} // namespace Sky::Clouds
