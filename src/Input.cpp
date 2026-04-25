#include "Gl/GlUtils.hpp"
#include "Input.hpp"

namespace Input {
    int g_windowWidth = 0;
    int g_windowHeight = 0;
    double g_mouseX = 0.0;
    double g_mouseY = 0.0;
    double g_scrollYOffset = 0.0;
    bool g_leftMouseDown = false;
    bool g_rightMouseDown = false;
    bool g_middleMouseDown = false;
    bool g_cursorVisible = true;
    bool g_windowMinimized = false;
    bool g_windowFocused = true;
    std::string g_textBuffer;

    std::bitset<350> g_keydownmap = {false};
    std::bitset<350> g_keypressedmap = {false};

    void Init() {
        if (!Gl::window) return;
        glfwSetScrollCallback(Gl::window, ScrollCallback);
        glfwSetWindowIconifyCallback(Gl::window, IconifyCallback);
        glfwSetWindowFocusCallback(Gl::window, FocusCallback);
        glfwSetWindowSizeCallback(Gl::window, ResizeCallback);
        // glfwSetCharCallback(Gl::window, CharCallback); This line brokes imgui input, so it temporiarly disabled
    }

    void IconifyCallback(GLFWwindow* window, int iconified) {
        g_windowMinimized = iconified == GLFW_TRUE;
    }

    void FocusCallback(GLFWwindow* window, int focused) {
        g_windowFocused = focused == GLFW_TRUE;
    }

    void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        g_scrollYOffset = yoffset;
    }

    void CharCallback(GLFWwindow* window, unsigned int codepoint) {
        g_textBuffer += static_cast<char>(codepoint);
    }

    void ResizeCallback(GLFWwindow* window, int width, int height) {
        g_windowHeight = width;
        g_windowWidth = width;
    }

    void ToggleCursor() {
        g_cursorVisible = !g_cursorVisible;
    }

    void SetCursorVisible(bool visible) {
        g_cursorVisible = visible;
    }

    void SetWindowSize(int width, int height) {
        g_windowWidth = width;
        g_windowHeight = height;
        glfwSetWindowSize(Gl::window, width, height);
    }

    void SetWindowTitle(const std::string& title) {
        glfwSetWindowTitle(Gl::window, title.c_str());
    }

    void SetWindowIcon(const GLFWimage* icon) {
        glfwSetWindowIcon(Gl::window, 1, icon);
    }

    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod) {
        if (key < 0 || key >= 349) return;

        switch (action) {
        case GLFW_PRESS:
            if (!g_keydownmap[key]) g_keypressedmap[key] = true;
            else g_keypressedmap[key] = false;
            g_keydownmap[key] = true;
            break;

        case GLFW_RELEASE:
            g_keydownmap[key] = false;
            g_keypressedmap[key] = false;
            break;

        case GLFW_REPEAT:
            g_keypressedmap[key] = false;
            g_keydownmap[key] = true;
            break;
        }
    }

    void PollEvents() {
        if (!Gl::window) return;
        // g_scrollYOffset = 0;
        g_textBuffer.clear();

        glfwPollEvents();

        if (!g_cursorVisible) glfwSetInputMode(Gl::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else glfwSetInputMode(Gl::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // Keyboard
        for (int i = 32; i < 349; i++) {
            if (glfwGetKey(Gl::window, i) == GLFW_PRESS) {
                if (!g_keydownmap[i]) g_keypressedmap[i] = true;
                else g_keypressedmap[i] = false;

                g_keydownmap[i] = true;
            } else {
                g_keypressedmap[i] = false;
                g_keydownmap[i] = false;
            }
        }

        glfwGetCursorPos(Gl::window, &g_mouseX, &g_mouseY);

        g_leftMouseDown = glfwGetMouseButton(Gl::window, GLFW_MOUSE_BUTTON_LEFT);
        g_rightMouseDown = glfwGetMouseButton(Gl::window, GLFW_MOUSE_BUTTON_RIGHT);
        g_middleMouseDown = glfwGetMouseButton(Gl::window, GLFW_MOUSE_BUTTON_MIDDLE);
    }

    std::string& GetTextBuffer() {
        return g_textBuffer;
    }

    bool IsCursorVisible() {
        return g_cursorVisible;
    }

    bool IsKeyDown(Gl::Key key) {
        return g_keydownmap[static_cast<uint16_t>(key)];
    }

    bool IsKeyPressed(Gl::Key key) {
        return g_keypressedmap[static_cast<uint16_t>(key)];
    }

    bool IsLeftMouseDown() {
        return g_leftMouseDown;
    }

    bool IsMiddleMouseDown() {
        return g_middleMouseDown;
    }

    bool IsRightMouseDown() {
        return g_rightMouseDown;
    }

    bool IsWindowMinimized() {
        return g_windowMinimized;
    }

    bool IsWindowFocused() {
        return g_windowFocused;
    }

    double GetMouseX() {
        return g_mouseX;
    }

    double GetMouseY() {
        return g_mouseY;
    }

    int GetWindowWidth() {
        return g_windowWidth;
    }

    int GetWindowHeight() {
        return g_windowHeight;
    }

    double GetScrollYOffset() {
        return g_scrollYOffset;
    }
} // namespace Input
