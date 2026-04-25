#include <glad/glad.h>
#include "TextureBuffer.hpp"
#include "Sky/Constants.hpp"
#include "Utils.hpp"
#include "Gl/GlUtils.hpp"

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Sky::Atm {

    Gl::Texture* make2d(uint16_t w, uint16_t h, GLenum internalFormat) {
        auto tex = new Gl::Texture();
        tex->create(w, h, GL_RGBA, internalFormat, GL_FLOAT);
        tex->setMinFilter(GL_LINEAR);
        tex->setMagFilter(GL_LINEAR);
        tex->setWrapMode(GL_CLAMP_TO_EDGE);
        return tex;
    }

    Gl::Texture3D* make3d(uint16_t w, uint16_t h, uint16_t d, GLenum internalFormat) {
        auto tex = new Gl::Texture3D();
        tex->create(w, h, d, GL_RGBA, internalFormat, GL_FLOAT);
        tex->setMinFilter(GL_LINEAR);
        tex->setMagFilter(GL_LINEAR);
        tex->setWrapMode(GL_CLAMP_TO_EDGE);
        return tex;
    }

    TextureBuffer::TextureBuffer(bool half_precision) {
        // 16F precision for the transmittance gives artifacts. Always use full.
        // Also using full for irradiance as they originally code did.

        GLint format = GL_RGBA32F;
        if (half_precision) format = GL_RGBA16F;

        delta_irradiance_texture = make2d(IRRADIANCE_WIDTH, IRRADIANCE_HEIGHT, GL_RGBA32F);
        delta_rayleigh_scattering_texture = make3d(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH, format);
        delta_mie_scattering_texture = make3d(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH, format);
        delta_scattering_density_texture = make3d(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH, format);

        transmittance_array[0] = make2d(TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, GL_RGBA32F);
        transmittance_array[1] = make2d(TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT, GL_RGBA32F);
        irradiance_array[0] = make2d(IRRADIANCE_WIDTH, IRRADIANCE_HEIGHT, GL_RGBA32F);
        irradiance_array[1] = make2d(IRRADIANCE_WIDTH, IRRADIANCE_HEIGHT, GL_RGBA32F);
        scattering_array[0] = make3d(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH, format);
        scattering_array[1] = make3d(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH, format);

        // delta_multiple_scattering_texture is only needed to compute scattering
        // order 3 or more, while delta_rayleigh_scattering_texture and
        // delta_mie_scattering_texture are only needed to compute double scattering.
        // Therefore, to save memory, we can store delta_rayleigh_scattering_texture
        // and delta_multiple_scattering_texture in the same GPU texture.
        delta_multiple_scattering_texture = delta_rayleigh_scattering_texture;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void TextureBuffer::clear(Gl::ComputeShader* shader2d, Gl::ComputeShader* shader3d) const {
        clear_texture(shader2d, delta_irradiance_texture);
        clear_texture(shader3d, delta_rayleigh_scattering_texture);
        clear_texture(shader3d, delta_mie_scattering_texture);
        clear_texture(shader3d, delta_scattering_density_texture);
        clear_texture(shader2d, transmittance_array[0]);
        clear_texture(shader2d, transmittance_array[1]);
        clear_texture(shader2d, irradiance_array[0]);
        clear_texture(shader2d, irradiance_array[1]);
        clear_texture(shader3d, scattering_array[0]);
        clear_texture(shader3d, scattering_array[1]);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void TextureBuffer::clear_texture(Gl::ComputeShader* shader, Gl::Texture* tex) {
        if (!tex || !shader) return;

        tex->bindImage(0, GL_READ_WRITE, 0);
        shader->use();
        shader->dispatch(tex->getWidth() / NUM_THREADS, tex->getHeight() / NUM_THREADS, 1);
        shader->barrier();
    }
    void TextureBuffer::clear_texture(Gl::ComputeShader* shader, Gl::Texture3D* tex) {
        if (!tex || !shader) return;

        tex->bindImage(0, GL_READ_WRITE, 0);
        shader->use();
        shader->dispatch(tex->getWidth() / NUM_THREADS, tex->getHeight() / NUM_THREADS, tex->getDepth() / NUM_THREADS);
        shader->barrier();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    TextureBuffer::~TextureBuffer() {
        PTR_SAFE_DELETE(delta_irradiance_texture);
        PTR_SAFE_DELETE(delta_rayleigh_scattering_texture);
        PTR_SAFE_DELETE(delta_mie_scattering_texture);
        PTR_SAFE_DELETE(delta_scattering_density_texture);
        // PTR_SAFE_DELETE(delta_multiple_scattering_texture);
        PTR_SAFE_DELETE(transmittance_array[0]);
        PTR_SAFE_DELETE(transmittance_array[1]);
        PTR_SAFE_DELETE(irradiance_array[0]);
        PTR_SAFE_DELETE(irradiance_array[1]);
        PTR_SAFE_DELETE(scattering_array[0]);
        PTR_SAFE_DELETE(scattering_array[1]);
    }
} // namespace Sky::Atm
