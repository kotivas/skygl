#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Gl {

    /* layout alignment by data types
                     // base alignment  // aligned offset
    float value;     // 4               // 0
    vec3 vector;     // 16              // 16  (offset must be multiple of 16 so 4->16)
    mat4 matrix;     // 16              // 32  (column 0)
                     // 16              // 48  (column 1)
                     // 16              // 64  (column 2)
                     // 16              // 80  (column 3)
    float values[3]; // 16              // 96  (values[0])
                     // 16              // 112 (values[1])
                     // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;     // 4               // 148

    use alignas(16) for vec3
     */

    // todo implement persistent mapping (no usage for now)
    class UniformBuffer {
    public:
        UniformBuffer();

        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;

        [[nodiscard]] bool isValid() const;

        void bind(uint32_t slot) const;
        void create(GLsizeiptr size);
        void setData(const void* data) const;
        void setSubData(const void* data, GLsizeiptr offset, GLsizeiptr size) const;

        ~UniformBuffer();

    private:
        GLuint m_handle;
        GLsizeiptr m_size;
    };


} // namespace Gl
