// OpenGL 4.5
#include <string>

#include "Sky/Atmosphere/AtmosphereModel.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Camera.hpp"
#include "Gl/GlUtils.hpp"
#include "Input.hpp"
#include <fstream>
#include "Utils.hpp"
#include "Sky/Constants.hpp"
#include "Sky/Sun.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <iostream>
#include <numeric>
#include <chrono>

#include "Gl/Shader.hpp"
#include "Sky/Clouds/CloudsModel.hpp"

struct Scene {
    // double time = 21600; // 6 am
    double time = 43200; // 12 am
    Sky::Sun sun;
    Gl::Shader skyShader;
    Gl::Shader cloudsShader;
    Gl::Shader composeShader;

    Camera camera;

    float exposure = 0.00076f; // 1/1300
    float gamma = 2.2f;

    float speed = 100.f / Sky::LenghtUnitInMeters;
} scene;

Sky::Atm::AtmosphereParameters atm_params = {
    // (computed so at to get 300 Dobson units of ozone - for this we divide 300 DU by the integral of
    // the ozone density profile defined below, which is equal to 15km).
    .maxOzoneNumberDensity = 300.0 * Sky::Atm::DOBSON_UNIT / 15000.0,
    .rayleigh = 1.24062e-6,
    .rayleighScaleHeight = 8000.0,
    .mieScaleHeight = 1200.0,
    .mieAngstromAlpha = 0.0,
    .mieAngstromBeta = 5.328e-3,
    .mieSingleScatteringAlbedo = 0.9,
    .miePhaseFunctionG = 0.9,
    .groundAlbedo = 0.1,
    .luminance = Sky::Atm::Luminance::Precomputed,
    .numScatteringOrders = 4,
};

Sky::Clouds::CloudsParameters cloudsParams = {
    .cloudLayerThickness = Sky::Clouds::CLOUD_LAYER_THICKNESS,
    .cloudLayerBottom = Sky::Clouds::CLOUD_LAYER_BOTTOM,
    .highCloudsHeight = Sky::Clouds::HIGH_CLOUDS_HEIGHT,
    .cirrusDensity = 0.05f,
    .altoDensity = 0.05f,
    .maxDistance = 128000.f,
    .coverage = 1.0f,

    .sigmaS = 0.01f,
    .sigmaA = 0.0f,

    .highCloudsScale = 32 * 1000.0,
    .weatherMapScale = 128 * 1000.0,
    .baseNoiseScale = 24 * 1000.0,
    .detailNoiseScale = 1.5 * 1000.0,
};

float windSpeed = 2.0f; // in m/s

Sky::Atm::AtmosphereModel atm_model;
Sky::Clouds::CloudsModel clouds_model;

GLuint skyFBO;
GLuint skyColor;
GLuint cloudsFBO;
GLuint cloudsColor;

uint32_t quadVao;
uint32_t quadVbo;

#define TIME_SPEED 30

constexpr int render_width = 1600;
constexpr int render_height = 900;
constexpr int sky_width = render_width;
constexpr int sky_height = render_height;
constexpr int clouds_width = render_width * 0.5;
constexpr int clouds_height = render_height * 0.5;

static bool timeStop = false;

void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(Gl::window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}
void DrawMetrics(double dt) {
    ImGui::Begin("Metrics");

    // precompute metrics
    const float t = fmod(scene.time, Sky::DAY_LENGHT);
    const int hour = static_cast<int>(t / 3600.0f);
    const int minute = static_cast<int>((t - hour * 3600.0f) / 60.0f);

    ImGui::Text("Time: %.2f %02d:%02d", scene.time, hour, minute);
    ImGui::Text("CPU Frametime: %.2f", dt * 1000.f);
    ImGui::Text("FPS: %.2f", 1 / dt);
    ImGui::Text("Pos: %.2f, %.2f, %.2f", scene.camera.position.x, scene.camera.position.y, scene.camera.position.z);
    ImGui::Text("Speed: %.1f", scene.speed);

    ImGui::End();
}
void DrawDebugOverlay(double dt) {
    ImGui::Begin("Settings");

    if (ImGui::CollapsingHeader("General")) {
        ImGui::InputFloat("Gamma", &scene.gamma);
        ImGui::InputFloat("Sens", &scene.camera.sensitivity);
        if (ImGui::SliderFloat("FOV", &scene.camera.fov, 55, 100, "%.1f")) scene.camera.calcProjMat(render_width, render_height);
        ImGui::InputFloat("Exposure", &scene.exposure, 0.00001, 0.0001, "%.5f");
        ImGui::InputFloat("Wind speed (m/s)", &windSpeed, 1.0f, 2.5f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Bruneton")) {
        ImGui::Text("maxOzoneNumberDensity: %.3e", atm_params.maxOzoneNumberDensity);
        ImGui::InputDouble("rayleigh", &atm_params.rayleigh, 0.001, 0.01, "%.15f");
        ImGui::InputDouble("rayleighScaleHeight", &atm_params.rayleighScaleHeight, 0.1, 1.0, "%.3f");
        ImGui::InputDouble("mieScaleHeight", &atm_params.mieScaleHeight, 0.1, 1.0, "%.3f");
        ImGui::InputDouble("mieAngstromAlpha", &atm_params.mieAngstromAlpha, 0.01, 0.1, "%.3f");
        ImGui::InputDouble("mieAngstromBeta", &atm_params.mieAngstromBeta, 0.0001, 0.001, "%.6f");
        ImGui::InputDouble("mieSingleScatteringAlbedo", &atm_params.mieSingleScatteringAlbedo, 0.01, 0.1, "%.3f");
        ImGui::InputDouble("miePhaseFunctionG", &atm_params.miePhaseFunctionG, 0.01, 0.1, "%.3f");
        ImGui::InputDouble("groundAlbedo", &atm_params.groundAlbedo, 0.01, 0.1, "%.3f");
        ImGui::InputInt("numScatteringOrders", &atm_params.numScatteringOrders);

        const char* luminanceOptions[] = {"None", "Approximate", "Precomputed"};
        int luminanceIndex = static_cast<int>(atm_params.luminance);
        if (ImGui::Combo("Luminance", &luminanceIndex, luminanceOptions, IM_ARRAYSIZE(luminanceOptions)))
            atm_params.luminance = static_cast<Sky::Atm::Luminance>(luminanceIndex);

        if (ImGui::Button("Compute model")) atm_model.initialize(atm_params);
    }
    if (ImGui::CollapsingHeader("Clouds")) {
        ImGui::Separator();
        ImGui::InputFloat("cloudLayerBottom", &cloudsParams.cloudLayerBottom, 10.0, 100.0, "%.1f");
        ImGui::InputFloat("cloudLayerThickness", &cloudsParams.cloudLayerThickness, 10.0, 100.0, "%.1f");
        ImGui::InputFloat("maxDistance", &cloudsParams.maxDistance, 10.0, 100.0, "%.2f");
        ImGui::SliderFloat("Cloud coverage", &cloudsParams.coverage, 0.f, 1.f, "%.2f");

        ImGui::SliderFloat("Cirrus density", &cloudsParams.cirrusDensity, 0.f, 1.f, "%.2f");
        ImGui::SliderFloat("Alto density", &cloudsParams.altoDensity, 0.f, 1.f, "%.2f");

        if (ImGui::Button("Update")) clouds_model.updateParameters(cloudsParams);
    }

    ImGui::End();
}

void LoadShaders() {
    PTR_SAFE_DELETE(atm_model.clear2DShader);
    PTR_SAFE_DELETE(atm_model.clear3DShader);
    PTR_SAFE_DELETE(atm_model.transmittanceShader);
    PTR_SAFE_DELETE(atm_model.directIrradianceShader);
    PTR_SAFE_DELETE(atm_model.indirectIrradianceShader);
    PTR_SAFE_DELETE(atm_model.multipleScatteringShader);
    PTR_SAFE_DELETE(atm_model.scatteringDensityShader);
    PTR_SAFE_DELETE(atm_model.singleScatteringShader);

    atm_model.clear2DShader = new Gl::ComputeShader("shaders/atmosphere/clear_2d_cs.glsl");
    atm_model.clear3DShader = new Gl::ComputeShader("shaders/atmosphere/clear_3d_cs.glsl");
    atm_model.transmittanceShader = new Gl::ComputeShader("shaders/atmosphere/compute_transmittance_cs.glsl");
    atm_model.directIrradianceShader = new Gl::ComputeShader("shaders/atmosphere/compute_direct_irradiance_cs.glsl");
    atm_model.indirectIrradianceShader = new Gl::ComputeShader("shaders/atmosphere/compute_indirect_irradiance_cs.glsl");
    atm_model.multipleScatteringShader = new Gl::ComputeShader("shaders/atmosphere/compute_multiple_scattering_cs.glsl");
    atm_model.scatteringDensityShader = new Gl::ComputeShader("shaders/atmosphere/compute_scattering_density_cs.glsl");
    atm_model.singleScatteringShader = new Gl::ComputeShader("shaders/atmosphere/compute_single_scattering_cs.glsl");

    scene.skyShader.load("shaders/ray.vert", "shaders/sky_pass.frag");
    scene.cloudsShader.load("shaders/ray.vert", "shaders/clouds_pass.frag");
    scene.composeShader.load("shaders/compose.vert", "shaders/compose.frag");
}

void SkyPass() {
    if (!scene.skyShader.isValid()) return;

    scene.skyShader.use();

    scene.skyShader.setUniform1f("uTime", scene.time);
    const auto viewNoTrans = glm::mat4(glm::mat3(scene.camera.viewMatrix));
    scene.skyShader.setUniformMat4fv("uView", viewNoTrans);
    scene.skyShader.setUniformMat4fv("uProjection", scene.camera.projectionMatrix);
    scene.skyShader.setUniform3f("uCameraPos", scene.camera.position);

    scene.skyShader.setUniform3f("uSunDir", scene.sun.direction);
    scene.skyShader.setUniform2f("uSunSize", glm::vec2(tan(Sky::SUN_ANGULAR_RADIUS), cos(Sky::SUN_ANGULAR_RADIUS)));

    atm_model.bind_uniform(&scene.skyShader);

    glViewport(0, 0, sky_width, sky_height);

    glBindFramebuffer(GL_FRAMEBUFFER, skyFBO);

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void CloudsPass() {
    if (!scene.cloudsShader.isValid()) return;

    scene.cloudsShader.use();
    scene.cloudsShader.setUniform1f("uTime", scene.time);
    scene.cloudsShader.setUniformMat4fv("uView", scene.camera.viewMatrix);
    scene.cloudsShader.setUniformMat4fv("uProjection", scene.camera.projectionMatrix);
    scene.cloudsShader.setUniform3f("uCameraPos", scene.camera.position);
    scene.cloudsShader.setUniform3f("uSunDir", scene.sun.direction);
    scene.cloudsShader.setUniform1f("uWindSpeed", windSpeed / Sky::LenghtUnitInMeters);

    clouds_model.bind(&scene.cloudsShader);
    atm_model.bind_uniform(&scene.cloudsShader);

    glViewport(0, 0, clouds_width, clouds_height);

    glBindFramebuffer(GL_FRAMEBUFFER, cloudsFBO);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void ComposePass() {
    scene.composeShader.use();

    scene.composeShader.setUniform1f("uGamma", scene.gamma);
    scene.composeShader.setUniform1f("uExposure", scene.exposure);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyColor);
    scene.composeShader.setUniform1i("skyTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cloudsColor);
    scene.composeShader.setUniform1i("cloudsTexture", 1);

    glViewport(0, 0, Input::GetWindowWidth(), Input::GetWindowHeight());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}
void ClearPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void LoadWeatherMap() {
    stbi_set_flip_vertically_on_load(true);
    int channels, x, y;
    unsigned char* weather = stbi_load("./res/weather.png", &x, &y, &channels, 3);
    assert(weather != nullptr);
    clouds_model.setWeatherMap(weather);
    stbi_image_free(weather);
}

void ProcessKeys(double dt) {
    if (Input::IsKeyPressed(Gl::Key::GraveAccent)) Input::ToggleCursor();
    if (Input::IsKeyPressed(Gl::Key::R)) {
        LoadShaders();
        LoadWeatherMap();
    }
    if (Input::IsKeyPressed(Gl::Key::P)) timeStop = !timeStop;


    if (Input::IsKeyDown(Gl::Key::PageUp)) scene.speed += 100 / Sky::LenghtUnitInMeters;
    if (Input::IsKeyDown(Gl::Key::PageDown)) scene.speed -= 100 / Sky::LenghtUnitInMeters;


    float speed = dt * scene.speed;
    if (Input::IsKeyDown(Gl::Key::LeftShift)) speed *= 10;
    if (Input::IsKeyDown(Gl::Key::RightShift)) speed *= 100;

    if (Input::IsKeyDown(Gl::Key::Space)) scene.camera.position += scene.camera.up * speed;
    if (Input::IsKeyDown(Gl::Key::LeftControl)) scene.camera.position -= scene.camera.up * speed;

    // WASD movement
    if (Input::IsKeyDown(Gl::Key::W)) scene.camera.position += scene.camera.forward * speed;
    if (Input::IsKeyDown(Gl::Key::S)) scene.camera.position -= scene.camera.forward * speed;
    glm::vec3 right = glm::normalize(glm::cross(scene.camera.forward, scene.camera.up));
    if (Input::IsKeyDown(Gl::Key::D)) scene.camera.position += right * speed;
    if (Input::IsKeyDown(Gl::Key::A)) scene.camera.position -= right * speed;

    // if (scene.camera.position.y <= 0) scene.camera.position.y = 0.1;

    // Time manipulation
    if (Input::IsKeyDown(Gl::Key::Right)) scene.time += 50.0 * dt * TIME_SPEED;
    if (Input::IsKeyDown(Gl::Key::Left)) scene.time -= 50.0 * dt * TIME_SPEED;
}

void InitCloud() {

    clouds_model.initialize(cloudsParams);

    LoadWeatherMap();

    stbi_set_flip_vertically_on_load(true);

    int channels, x, y;
    unsigned char* cirrus = stbi_load("./res/cirrus.png", &x, &y, &channels, 0);
    std::cout << std::format("cirrus.png is loaded ({0}x{1}x{2})", x, y, channels) << std::endl;
    assert(cirrus);
    unsigned char* alto = stbi_load("./res/alto.png", &x, &y, &channels, 0);
    std::cout << std::format("alto.png is loaded ({0}x{1}x{2})", x, y, channels) << std::endl;
    assert(alto);

    int res = Sky::Clouds::HIGH_CLOUDS_MAP_SIZE * Sky::Clouds::HIGH_CLOUDS_MAP_SIZE;
    auto* combined = new uint8_t[res * 2];
    for (int i = 0; i < res; i++) {
        combined[i * 2 + 0] = cirrus[i]; // R = cirrus
        combined[i * 2 + 1] = alto[i];   // G = alto
    }

    clouds_model.setHighCloudsMap(combined);

    stbi_image_free(cirrus);
    stbi_image_free(alto);
    delete[] combined;

    const Gl::ComputeShader base("./shaders/clouds/base_noise.comp");
    const Gl::ComputeShader detail("./shaders/clouds/detail_noise.comp");
    clouds_model.generateBaseNoise(base);
    clouds_model.generateDetailNoise(detail);
}

void LoadIcon(const std::string& path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force 4 channels (RGBA)

    if (pixels) {
        auto* image = new GLFWimage();
        image->height = height;
        image->pixels = pixels;
        image->width = width;

        Input::SetWindowIcon(image);

        stbi_image_free(pixels);
    }
}

int main() {
    Gl::Init();
    Input::SetWindowSize(render_width, render_height);
    InitImGui();

    glfwSwapInterval(1); // vsync

    LoadIcon("res/icon.png");

    Input::Init();
    Input::SetCursorVisible(false);

    LoadShaders();

    Gl::CreateQuadVO(quadVao, quadVbo);
    Gl::CreateFrameBuffer(skyFBO, skyColor, GL_RGBA32F, GL_RGBA, sky_width, sky_height);
    Gl::CreateFrameBuffer(cloudsFBO, cloudsColor, GL_RGBA16F, GL_RGBA, clouds_width, clouds_height);

    atm_model.initialize(atm_params);
    InitCloud();

    scene.camera = Camera(0.1, 55, {0, 0, 0});
    scene.camera.calcProjMat(render_width, render_height);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(Gl::window)) {
        // -------------- pre frame process --------------

        Input::PollEvents();

        const double dt = glfwGetTime() - lastTime;
        lastTime = glfwGetTime();

        if (Input::IsWindowMinimized() || !Input::IsWindowFocused()) continue;

        ClearPass();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // -------------- process --------------
        if (!timeStop) scene.time += dt * TIME_SPEED;

        ProcessKeys(dt);

        scene.camera.calcViewMat();
        scene.camera.calcForwardMat();

        if (!Input::IsCursorVisible()) scene.camera.updateControls(dt);

        scene.sun.update(scene.time, 80); // 157

        // -------------- render --------------

        SkyPass();
        CloudsPass();
        ComposePass();
        // imgui
        DrawDebugOverlay(dt);
        DrawMetrics(dt);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // ..

        glfwSwapBuffers(Gl::window);
        glfwPollEvents();
    }


    // ~
    glDeleteVertexArrays(1, &quadVao);
    glDeleteBuffers(1, &quadVbo);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
