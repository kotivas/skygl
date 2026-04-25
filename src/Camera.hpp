#pragma once
#include "Input.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_geometric.hpp>

struct Camera {
    Camera(float sens = 0.1, float fov = 45.f, glm::vec3 position = {0, 0, 0}, float znear = 0.1, float zfar = 100) :
        position(position), yaw(0), pitch(0), fov(fov), nearPlane(znear), farPlane(zfar), sensitivity(sens), forward(0, 0, 1), up(0, 1, 0), _lastx(0),
        _lasty(0) {}

    glm::vec3 position;
    float yaw;
    float pitch;
    float fov;

    float nearPlane;
    float farPlane;

    float sensitivity;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 forward;
    glm::vec3 up;

    double _lastx, _lasty;

    void updateControls(double dt) { // ! bad
        // mouse
        double xoffset = Input::GetMouseX() - _lastx;
        double yoffset = Input::GetMouseY() - _lasty;

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += -yoffset; // inverse

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        _lastx = Input::GetMouseX();
        _lasty = Input::GetMouseY();
    }

    void calcProjMat(const float res_x, const float res_y) {
        projectionMatrix = glm::perspective(glm::radians(fov), res_x / res_y, nearPlane, farPlane);
    }

    void calcViewMat() {
        viewMatrix = glm::lookAt(position, position + forward, {0, 1, 0});
    }

    void calcForwardMat() {
        forward = {cos(glm::radians(pitch)) * cos(glm::radians(yaw)), sin(glm::radians(pitch)), cos(glm::radians(pitch)) * sin(glm::radians(yaw))};
    }
};
