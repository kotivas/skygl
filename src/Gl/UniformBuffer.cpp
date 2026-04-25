#include "UniformBuffer.hpp"

#include "GlUtils.hpp"

namespace Gl {

    UniformBuffer::UniformBuffer() : m_handle(0), m_size(0) {}

    void UniformBuffer::bind(uint32_t slot) const {
        assert(isValid() && "UniformBuffer::bind called before create()");
        glBindBufferBase(GL_UNIFORM_BUFFER, slot, m_handle);
    }

    bool UniformBuffer::isValid() const {
        return m_handle != 0;
    }

    void UniformBuffer::create(GLsizeiptr size) {
        assert(!isValid() && "UniformBuffer::create called on an already existing buffer");
        assert(size < Gl::MAX_UNIFORM_BLOCK_SIZE && "UniformBuffer::create size < Gl::MAX_UNIFORM_BLOCK_SIZE");

        m_size = size;

        glCreateBuffers(1, &m_handle);
        glNamedBufferData(m_handle, size, nullptr, GL_DYNAMIC_DRAW);
    }

    void UniformBuffer::setData(const void* data) const {
        assert(isValid() && "UniformBuffer::setData called before create() ");
        assert(m_size != sizeof(data) && "UniformBuffer::setData size of new data differs from the internal ubo size");
        glNamedBufferSubData(m_handle, 0, m_size, data);
    }

    void UniformBuffer::setSubData(const void* data, GLsizeiptr size, GLintptr offset) const {
        assert(isValid() && "UniformBuffer::setSubData called before create() ");
        assert(size < m_size && "UniformBuffer::setSubData sub data size > size of whole UBO");
        glNamedBufferSubData(m_handle, offset, size, data);
    }

    UniformBuffer::~UniformBuffer() {
        if (m_handle) glDeleteBuffers(1, &m_handle);
    }
} // namespace Gl
