#include "GlUtils.hpp"

#include <iostream>


namespace Gl {
    GLint MAX_UNIFORM_BLOCK_SIZE = 0;

    void UpdateDeviceLimits() {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &MAX_UNIFORM_BLOCK_SIZE);
        std::cout << "[DEVICE] MAX UNIFORM BLOCK SIZE = " << MAX_UNIFORM_BLOCK_SIZE << std::endl;
    }

    void CreateFrameBuffer(GLuint& fbo, GLuint& texture, GLenum internalFormat, GLenum format, int width, int height) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // needed
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "unable to create sky FBO" << std::endl;
    }

    void CreateQuadVO(uint32_t& vao, uint32_t& vbo) {
        // Each vertex: x, y, u, v
        constexpr float quadVertices[] = {
            // positions   // tex coords
            -1.f, 1.f, 0.f, 1.f,  // top-left
            -1.f, -1.f, 0.f, 0.f, // bottom-left
            1.f, 1.f, 1.f, 1.f,   // top-right
            1.f, -1.f, 1.f, 0.f   // bottom-right
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // position attribute (x, y)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        // texcoord attribute (u, v)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }


    void Init() {
        if (!glfwInit()) {
            std::cout << "[GLFW] Failed to initialize GLFW" << std::endl;
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, DEFAULT_WINDOW_TITLE, DEFAULT_MONITOR, nullptr);
        if (!window) {
            std::cout << "[GLFW] Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, Viewport);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "[GLAD] Failed to initialize GLAD" << std::endl;
        }

        UpdateDeviceLimits();
    }

    void Viewport(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

} // namespace Gl
