#pragma once
#include "Gl/Shader.hpp"
#include "Gl/Texture.hpp"

namespace Sky::Atm {

    struct TextureBuffer {
        Gl::Texture* delta_irradiance_texture = nullptr;
        Gl::Texture3D* delta_rayleigh_scattering_texture = nullptr;
        Gl::Texture3D* delta_mie_scattering_texture = nullptr;
        Gl::Texture3D* delta_scattering_density_texture = nullptr;

        Gl::Texture3D* delta_multiple_scattering_texture = nullptr;

        Gl::Texture* transmittance_array[2] = {nullptr, nullptr};
        Gl::Texture* irradiance_array[2] = {nullptr, nullptr};
        Gl::Texture3D* scattering_array[2] = {nullptr, nullptr};

        TextureBuffer(bool half_precision);

        void clear(Gl::ComputeShader* shader2d, Gl::ComputeShader* shader3d) const;

        ~TextureBuffer();

    private:
        static void clear_texture(Gl::ComputeShader* shader, Gl::Texture* tex);
        static void clear_texture(Gl::ComputeShader* shader, Gl::Texture3D* tex);
    };

} // namespace Sky::Atm
