#pragma once
#include "Gl/Shader.hpp"
#include "Gl/Texture.hpp"
#include "Gl/UniformBuffer.hpp"

namespace Sky::Clouds {

    struct CloudsParameters {
        float cloudLayerThickness; // Volumetric cloud height thickness (m)
        float cloudLayerBottom;    // Volumetric cloud Height bottom (m)
        float highCloudsHeight;    // High-Altitude clouds height (m)
        float cirrusDensity;       // Density of cirrus clouds
        float altoDensity;         // Density of alto clouds
        float maxDistance;         // Volumetric cloud max render distance
        float coverage;            // Volumetric cloud coverage percentage

        float sigmaS; // Volumetric cloud scattering factor
        float sigmaA; // Volumetric cloud absorption factor

        float highCloudsScale;
        float weatherMapScale;
        float baseNoiseScale;
        float detailNoiseScale;

    }; // (20 bytes)

    class CloudsModel {
    public:
        CloudsModel();

        void initialize(const CloudsParameters& params);

        void generateBaseNoise(const Gl::ComputeShader& shader) const;
        void generateDetailNoise(const Gl::ComputeShader& shader) const;

        void setWeatherMap(const uint8_t* data) const;
        void setHighCloudsMap(const uint8_t* data) const;

        void bind(Gl::Shader* shader) const;

        void updateParameters(const CloudsParameters& params);

        ~CloudsModel();

    private:
        Gl::UniformBuffer uniformBuffer;

        CloudsParameters m_params;
        Gl::Texture3D* m_baseNoise;
        Gl::Texture3D* m_detailNoise;
        Gl::Texture* m_weatherMap;
        Gl::Texture* m_highCloudsMap;
    };

} // namespace Sky::Clouds
