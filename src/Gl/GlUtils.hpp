#pragma once
#include "Texture.hpp"
#include <GLFW/glfw3.h>

#include <filesystem>
#include <string>

#define DEFAULT_WINDOW_TITLE "SkyGL"
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define DEFAULT_MONITOR 0

namespace Gl {
    void Viewport(GLFWwindow* window, int width, int height);

    void CreateQuadVO(uint32_t& vao, uint32_t& vbo);
    void CreateFrameBuffer(GLuint& fbo, GLuint& texture, GLenum internalFormat, GLenum format, int width, int height);

    void Init();

    inline GLFWwindow* window = nullptr;

    extern GLint MAX_UNIFORM_BLOCK_SIZE;
} // namespace Gl
