#include "Texture.hpp"

namespace Gl {

    uint16_t GetChannelsFromFormat(GLenum format) {
        switch (format) {
        case GL_RED:
            return 1;
        case GL_RG:
            return 2;
        case GL_RGB:
            return 3;
        case GL_RGBA:
            return 4;
        case GL_RED_INTEGER:
            return 1;
        case GL_RG_INTEGER:
            return 2;
        case GL_RGB_INTEGER:
            return 3;
        case GL_RGBA_INTEGER:
            return 4;
        case GL_DEPTH_COMPONENT:
            return 1;
        case GL_DEPTH_STENCIL:
            return 2;
        case GL_STENCIL_INDEX:
            return 1;
        default:
            return 0;
        }
    }

    /// =========== BASE TETURE ==========
    bool BaseTexture::isValid() const {
        return m_handle != 0;
    }
    GLuint BaseTexture::getHandle() const {
        return m_handle;
    }
    uint16_t BaseTexture::getChannels() const {
        return m_channels;
    }
    GLuint BaseTexture::getMipmapLevel() const {
        return m_mipmapLevel;
    }
    GLenum BaseTexture::getInternalFormat() const {
        return m_internalFormat;
    }
    GLenum BaseTexture::getFormat() const {
        return m_format;
    }

    void BaseTexture::generateMipMaps() const {
        glGenerateTextureMipmap(m_handle);
    }

    void BaseTexture::setMinFilter(GLenum filter) const {
        glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filter);
    }
    void BaseTexture::setMagFilter(GLenum filter) const {
        glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, filter);
    }

    void BaseTexture::bindImage(GLuint slot, GLenum access, GLuint layer, GLboolean layered) const {
        glBindImageTexture(slot, m_handle, 0, layered, layer, access, m_internalFormat);
    }

    void BaseTexture::clear() const {
        if (!isValid()) return;
        const GLfloat color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int level = 0; level < m_mipmapLevel; ++level) glClearTexImage(m_handle, level, m_format, m_type, color);
    }

    /// =========== TEXTURE =============

    Texture::Texture() : m_width(0), m_height(0) {}
    Texture::Texture(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}


    void Texture::bind(uint32_t slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_handle);
    }

    void Texture::setWrapMode(GLenum wrapMode) const {
        glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, wrapMode);
        glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, wrapMode);
    }

    void Texture::setData(const void* data, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY) const {
        glTextureSubImage2D(m_handle, 0, offsetX, offsetY, width, height, m_format, m_type, data);
    }

    uint32_t Texture::getWidth() const {
        return m_width;
    }
    uint32_t Texture::getHeight() const {
        return m_height;
    }

    GLenum Texture::getTextureType() const {
        return GL_TEXTURE_2D;
    }

    void Texture::create(uint32_t width, uint32_t height, GLenum format, GLenum internalFormat, GLenum type, bool useMipmaps) {
        GLuint mipmapLevel = 1;
        if (useMipmaps) mipmapLevel = static_cast<GLuint>(std::floor(std::log2(std::max(width, height)))) + 1;

        m_width = width;
        m_height = height;
        m_mipmapLevel = mipmapLevel;
        m_internalFormat = internalFormat;
        m_format = format;
        m_channels = GetChannelsFromFormat(format);
        m_type = type;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
        glTextureStorage2D(m_handle, mipmapLevel, internalFormat, width, height);
    }

    Texture::~Texture() {
        glDeleteTextures(1, &m_handle);
    }

    /// =========== 3D TEXTURE =============


    Texture3D::Texture3D() : m_width(0), m_height(0), m_depth(0) {}
    Texture3D::Texture3D(uint32_t width, uint32_t height, uint32_t depth) : m_width(width), m_height(height), m_depth(depth) {}


    void Texture3D::bind(uint32_t slot) {
        if (isValid()) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_3D, m_handle);
        }
    }

    void Texture3D::setWrapMode(GLenum wrapMode) const {
        glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, wrapMode);
        glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, wrapMode);
        glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, wrapMode);
    }

    void Texture3D::setData(const void* data, uint32_t width, uint32_t height, uint32_t depth, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ) const {
        glTextureSubImage3D(m_handle, 0, offsetX, offsetY, offsetZ, width, height, depth, m_format, m_type, data);
    }

    uint32_t Texture3D::getWidth() const {
        return m_width;
    }
    uint32_t Texture3D::getHeight() const {
        return m_height;
    }

    uint32_t Texture3D::getDepth() const {
        return m_depth;
    }

    GLenum Texture3D::getTextureType() const {
        return GL_TEXTURE_3D;
    }

    void Texture3D::create(uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum internalFormat, GLenum type, bool useMipmaps) {
        GLuint mipmapLevel = 1;
        if (useMipmaps) mipmapLevel = static_cast<GLuint>(std::floor(std::log2(std::max({width, height, depth})))) + 1;

        m_width = width;
        m_height = height;
        m_depth = depth;
        m_mipmapLevel = mipmapLevel;
        m_internalFormat = internalFormat;
        m_format = format;
        m_channels = GetChannelsFromFormat(format);
        m_type = type;

        glCreateTextures(GL_TEXTURE_3D, 1, &m_handle);
        glTextureStorage3D(m_handle, mipmapLevel, internalFormat, width, height, depth);
    }

    Texture3D::~Texture3D() {
        glDeleteTextures(1, &m_handle);
    }

} // namespace Gl
