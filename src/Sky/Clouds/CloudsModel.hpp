#pragma once
#include "Gl/Shader.hpp"
#include "Gl/Texture.hpp"
#include "Gl/UniformBuffer.hpp"

namespace Sky::Clouds {

    struct CloudsParameters {
        float maxDistance; // Volumetric cloud max render distance

        float sigmaS; // Volumetric cloud scattering factor
        float sigmaA; // Volumetric cloud absorption factor

        float cloudLayerThickness;
        float cloudLayerBottom;
        float highCloudsHeight;

        float highCloudsScale;
        float weatherMapScale;
        float baseNoiseScale;
        float detailNoiseScale;
    };

    class CloudsModel {
    public:
        CloudsModel();

        void initialize(const CloudsParameters& params);

        void generateBaseNoise(const Gl::ComputeShader* shader) const;
        void generateDetailNoise(const Gl::ComputeShader* shader) const;

        void swapWeatherMap();
        void setNextWeatherMap(const float* next) const;

        void setHighCloudsMap(const uint8_t* data) const;

        void bind(Gl::Shader* shader) const;

        void updateParameters(const CloudsParameters& params);

        [[nodiscard]] Gl::Texture* getCurrentWeatherMap() const;
        [[nodiscard]] Gl::Texture* getNextWeatherMap() const;

        ~CloudsModel();

    private:
        Gl::UniformBuffer _paramsUBO;
        CloudsParameters _params{};

        Gl::Texture3D* _baseNoise;
        Gl::Texture3D* _detailNoise;

        Gl::Texture* _currentWeatherMap;
        Gl::Texture* _nextWeatherMap;

        Gl::Texture* _highCloudsMap;
    };

} // namespace Sky::Clouds
