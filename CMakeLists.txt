cmake_minimum_required(VERSION 4.0)
project(skygl C CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(Stb REQUIRED)

# ===== Compile & Link =====
set(SOURCE src/main.cpp
        src/Utils.cpp
        src/Input.cpp
        src/Sky/Atmosphere/AtmosphereModel.cpp
        src/Sky/Atmosphere/TextureBuffer.cpp
        src/Sky/Clouds/CloudsModel.cpp
        src/Gl/GlUtils.cpp
        src/Gl/Shader.cpp
        src/Gl/Texture.cpp
        src/Gl/UniformBuffer.cpp
)

if (WIN32)
    list(APPEND SOURCES res/resource.rc)
endif ()

add_executable(skygl ${SOURCE})
target_include_directories(skygl PRIVATE src)
target_include_directories(skygl PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(skygl PRIVATE OpenGL::GL glm::glm glfw glad::glad imgui::imgui)

