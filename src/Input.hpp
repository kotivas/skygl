#pragma once
#include <bitset>
#include <GLFW/glfw3.h>
#include "Gl/Keys.h"

namespace Input {
    void Init();
    void PollEvents();
    void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void CharCallback(GLFWwindow* window, unsigned int codepoint);
    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
    void IconifyCallback(GLFWwindow* window, int iconified);
    void FocusCallback(GLFWwindow* window, int focused);
    void ResizeCallback(GLFWwindow* window, int width, int height);

    bool IsKeyDown(Gl::Key key);
    bool IsKeyPressed(Gl::Key key);
    bool IsLeftMouseDown();
    bool IsMiddleMouseDown();
    bool IsRightMouseDown();
    bool IsCursorVisible();
    bool IsWindowMinimized();
    bool IsWindowFocused();
    std::string& GetTextBuffer();

    void SetWindowSize(int width, int height);
    void SetWindowTitle(const std::string& title);
    void SetWindowIcon(const GLFWimage* icon);
    int GetWindowWidth();
    int GetWindowHeight();

    void ToggleCursor();
    void SetCursorVisible(bool visible);

    double GetMouseX();
    double GetMouseY();
    double GetScrollYOffset();
} // namespace Input
