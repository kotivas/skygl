#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Gl {

    class BaseShader {
    protected:
        GLuint m_handle = 0;

    public:
        void use() const;

        void setUniform1i(const std::string& name, int v0);
        void setUniform2i(const std::string& name, int v0, int v1);
        void setUniform3i(const std::string& name, int v0, int v1, int v2);
        void setUniform4i(const std::string& name, int v0, int v1, int v2, int v3);
        void setUniform1f(const std::string& name, float v0);
        void setUniform2f(const std::string& name, float v0, float v1);
        void setUniform2f(const std::string& name, glm::vec2 v);
        void setUniform3f(const std::string& name, float v0, float v1, float v2);
        void setUniform3f(const std::string& name, glm::vec3 v);
        void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
        void setUniform4f(const std::string& name, glm::vec4 v);
        void setUniformMat4fv(const std::string& name, glm::mat4 value, bool transpose = false);

        [[nodiscard]] bool isValid() const;

        virtual ~BaseShader() = default;

    private:
        GLint getUniformLocation(const std::string& name);
        std::unordered_map<std::string, GLint> m_uniformsCache;
    };

    class Shader : public BaseShader {
    public:
        Shader() = default;
        Shader(const std::string& vertexPath, const std::string& fragmentPath);

        void load(const std::string& vertexPath, const std::string& fragmentPath);
        void unload();

        ~Shader();

    private:
        void makeShader(const std::string& vertex_src, const std::string& fragment_src);
    };

    class ComputeShader : public BaseShader {
    public:
        ComputeShader() = default;
        ComputeShader(const std::string& path);

        void load(const std::string& path);

        // ...
        void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const;
        void barrier(GLbitfield barriers = GL_ALL_BARRIER_BITS) const;
        //

        void unload();

        ~ComputeShader();

    private:
        void makeShader(const std::string& src);
    };
} // namespace Gl
