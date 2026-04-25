#include "Shader.hpp"

#include "Utils.hpp"

#include <filesystem>
#include <iostream>
#include <ostream>
#include <unordered_set>
#include <glm/gtc/type_ptr.hpp>

namespace Gl {

    /// Parses includes
    std::string ParseShaderSource(const std::filesystem::path& filePath, std::unordered_set<std::string>& includedFiles) {
        std::filesystem::path absolutePath = std::filesystem::absolute(filePath);

        if (includedFiles.contains(absolutePath.generic_string())) return "";

        includedFiles.insert(absolutePath.generic_string());

        std::string source = Utils::ReadFromFile(absolutePath.generic_string());
        std::stringstream input(source);
        std::stringstream output;

        std::string line;
        int lineNumber = 1;

        while (std::getline(input, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            if (trimmed.starts_with("#include")) {
                size_t firstQuote = trimmed.find('<');
                size_t lastQuote = trimmed.find('>', firstQuote + 1);

                if (firstQuote == std::string::npos || lastQuote == std::string::npos) {
                    std::cout << "Invalid #include syntax in " << absolutePath.generic_string() << std::endl;
                    return "";
                }

                std::string includeFile = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);

                std::filesystem::path includePath = absolutePath.parent_path() / includeFile;

                output << "\n#line 1 \"" << includePath.generic_string() << "\"\n";
                output << ParseShaderSource(includePath, includedFiles);
                output << "\n#line " << (lineNumber + 1) << " \"" << absolutePath.generic_string() << "\"\n";
            } else {
                output << line << '\n';
            }
            lineNumber++;
        }
        return output.str();
    }

    /// compiles shader
    GLuint CompileShader(const GLenum shader_type, const char* source) {
        const GLuint loc = glCreateShader(shader_type);

        glShaderSource(loc, 1, &source, nullptr);
        glCompileShader(loc);

        GLint success;
        glGetShaderiv(loc, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(loc, 512, nullptr, infoLog);
            std::cout << "Shader compilation failed: " << infoLog << std::endl;
            return 0;
        }
        return loc;
    }

    /// =========== Compute Shader ===========

    void ComputeShader::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const {
        glDispatchCompute(groupsX, groupsY, groupsZ);
    }

    void ComputeShader::barrier(GLbitfield barriers) const {
        glMemoryBarrier(barriers);
    }

    ComputeShader::ComputeShader(const std::string& path) {
        load(path);
    }

    void ComputeShader::load(const std::string& path) {
        if (isValid()) unload();

        std::unordered_set<std::string> includedFiles;
        const std::string cs = ParseShaderSource(path, includedFiles);

        makeShader(cs);
        if (m_handle) std::cout << path << " is loaded!" << std::endl;
    }

    void ComputeShader::makeShader(const std::string& src) {
        const GLuint cs = CompileShader(GL_COMPUTE_SHADER, src.c_str());

        if (!cs) return;

        m_handle = glCreateProgram();

        glAttachShader(m_handle, cs);

        glLinkProgram(m_handle);

        GLint success;
        glGetProgramiv(m_handle, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_handle, 512, nullptr, infoLog);
            std::cout << "Link failed: " << infoLog << std::endl;
            glDeleteProgram(m_handle);
            m_handle = 0;
        }

        glValidateProgram(m_handle);

        glDeleteShader(cs);
    }

    void ComputeShader::unload() {
        if (m_handle) {
            glDeleteProgram(m_handle);
            m_handle = 0;
        }
    }

    ComputeShader::~ComputeShader() {
        unload();
    }

    /// =========== Shader ===========

    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        load(vertexPath, fragmentPath);
    }

    void Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
        if (isValid()) unload();

        std::unordered_set<std::string> includedFiles;
        std::string vertex = ParseShaderSource(vertexPath, includedFiles);
        includedFiles.clear();
        std::string fragment = ParseShaderSource(fragmentPath, includedFiles);

        makeShader(vertex, fragment);
        if (m_handle) std::cout << vertexPath << " " << fragmentPath << " is loaded!" << std::endl;
    }

    void Shader::makeShader(const std::string& vertex_src, const std::string& fragment_src) {
        const GLuint vs = CompileShader(GL_VERTEX_SHADER, vertex_src.c_str());
        const GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_src.c_str());

        if (!vs || !fs) return;

        m_handle = glCreateProgram();

        glAttachShader(m_handle, vs);
        glAttachShader(m_handle, fs);

        glLinkProgram(m_handle);

        GLint success;
        glGetProgramiv(m_handle, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_handle, 512, nullptr, infoLog);
            std::cout << "Link failed: " << infoLog << std::endl;
            glDeleteProgram(m_handle);
            m_handle = 0;
        }

        glValidateProgram(m_handle);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void Shader::unload() {
        if (m_handle) {
            glDeleteProgram(m_handle);
            m_handle = 0;
        }
    }

    Shader::~Shader() {
        unload();
    }

    /// =========== BaseShader ===========

    void BaseShader::use() const {
        glUseProgram(m_handle);
    }

    bool BaseShader::isValid() const {
        return m_handle != 0;
    }

    GLint BaseShader::getUniformLocation(const std::string& name) {
        if (m_uniformsCache.contains(name)) return m_uniformsCache[name];

        const GLint uniform_loc = glGetUniformLocation(m_handle, name.c_str());
        m_uniformsCache[name] = uniform_loc;
        return uniform_loc;
    }

    // ------------------------------ INT ------------------------------
    void BaseShader::setUniform1i(const std::string& name, int v0) {
        glUniform1i(getUniformLocation(name), v0);
    }
    void BaseShader::setUniform2i(const std::string& name, int v0, int v1) {
        glUniform2i(getUniformLocation(name), v0, v1);
    }
    void BaseShader::setUniform3i(const std::string& name, int v0, int v1, int v2) {
        glUniform3i(getUniformLocation(name), v0, v1, v2);
    }
    void BaseShader::setUniform4i(const std::string& name, int v0, int v1, int v2, int v3) {
        glUniform4i(getUniformLocation(name), v0, v1, v2, v3);
    }
    // ------------------------------ FLOAT ------------------------------
    void BaseShader::setUniform1f(const std::string& name, float v0) {
        glUniform1f(getUniformLocation(name), v0);
    }
    void BaseShader::setUniform2f(const std::string& name, float v0, float v1) {
        glUniform2f(getUniformLocation(name), v0, v1);
    }
    void BaseShader::setUniform2f(const std::string& name, glm::vec2 v) {
        glUniform2f(getUniformLocation(name), v.x, v.y);
    }
    void BaseShader::setUniform3f(const std::string& name, float v0, float v1, float v2) {
        glUniform3f(getUniformLocation(name), v0, v1, v2);
    }
    void BaseShader::setUniform3f(const std::string& name, glm::vec3 v) {
        glUniform3f(getUniformLocation(name), v.x, v.y, v.z);
    }
    void BaseShader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
        glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
    }
    void BaseShader::setUniform4f(const std::string& name, glm::vec4 v) {
        glUniform4f(getUniformLocation(name), v.x, v.y, v.z, v.w);
    }
    // ------------------------------ MATRIX ------------------------------
    void BaseShader::setUniformMat4fv(const std::string& name, glm::mat4 value, bool transpose) {
        glUniformMatrix4fv(getUniformLocation(name), 1, transpose, glm::value_ptr(value));
    }
} // namespace Gl
