#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Gl {

    /*
[TODO] GL_TEXTURE_2D: fundamental
[TODO] GL_TEXTURE_3D: fundamental
[MAYBE] GL_TEXTURE_CUBE_MAP: fundamental 6x2D
[MAYBE] GL_TEXTURE_2D_MULTISAMPLE: widespread MSAA
[MAYBE] GL_TEXTURE_2D_ARRAY: widespread CSM
[X] GL_TEXTURE_CUBE_MAP_ARRAY: rare
[X] GL_TEXTURE_2D_MULTISAMPLE_ARRAY: rare
[X] GL_TEXTURE_1D: LEGACE
[X] GL_TEXTURE_BUFFER: LEGACY
[X] GL_TEXTURE_RECTANGLE: DEPRECATED
[X] GL_TEXTURE_1D_ARRAY: DEPRECATED
    */

    class BaseTexture {
    protected:
        GLuint m_mipmapLevel;
        uint16_t m_channels;
        GLenum m_format;
        GLenum m_internalFormat;
        GLenum m_type;
        GLuint m_handle;

    public:
        BaseTexture() : m_handle(0), m_mipmapLevel(1), m_channels(0), m_format(GL_NONE), m_internalFormat(GL_NONE), m_type(GL_NONE) {}

        virtual ~BaseTexture() = default;

        virtual void bind(uint32_t slot) = 0;
        virtual void setWrapMode(GLenum wrapMode) const = 0;
        [[nodiscard]] virtual GLenum getTextureType() const = 0;

        void setMinFilter(GLenum filter) const;
        void setMagFilter(GLenum filter) const;
        void clear() const;
        void bindImage(GLuint slot, GLenum access, GLuint layer = 0, GLboolean layered = GL_TRUE) const;

        void generateMipMaps() const;
        [[nodiscard]] bool isValid() const;
        [[nodiscard]] uint16_t getChannels() const;
        [[nodiscard]] GLenum getFormat() const;
        [[nodiscard]] GLenum getInternalFormat() const;
        [[nodiscard]] GLuint getMipmapLevel() const;

        [[nodiscard]] GLuint getHandle() const;
    };

    class Texture final : public BaseTexture {
    public:
        Texture();
        Texture(uint32_t width, uint32_t height);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        void create(uint32_t width, uint32_t height, GLenum format, GLenum internalFormat, GLenum type, bool useMipmaps = false);
        void setData(const void* data, uint32_t width, uint32_t height, uint32_t offsetX = 0, uint32_t offsetY = 0) const;

        void bind(uint32_t slot) override;
        void setWrapMode(GLenum wrapMode) const override;
        [[nodiscard]] GLenum getTextureType() const override;

        [[nodiscard]] uint32_t getWidth() const;
        [[nodiscard]] uint32_t getHeight() const;

        ~Texture() override;

    private:
        uint32_t m_width;
        uint32_t m_height;
    };

    class Texture3D final : public BaseTexture {
    public:
        Texture3D();
        Texture3D(uint32_t width, uint32_t height, uint32_t depth);
        Texture3D(const Texture3D&) = delete;
        Texture3D& operator=(const Texture3D&) = delete;

        void create(uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum internalFormat, GLenum type, bool useMipmaps = false);
        void setData(const void* data, uint32_t width, uint32_t height, uint32_t depth, uint32_t offsetX = 0, uint32_t offsetY = 0, uint32_t offsetZ = 0) const;

        void bind(uint32_t slot) override;
        void setWrapMode(GLenum wrapMode) const override;
        [[nodiscard]] GLenum getTextureType() const override;

        [[nodiscard]] uint32_t getWidth() const;
        [[nodiscard]] uint32_t getHeight() const;
        [[nodiscard]] uint32_t getDepth() const;

        ~Texture3D() override;

    private:
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_depth;
    };
} // namespace Gl
