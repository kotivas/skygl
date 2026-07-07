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
#include <chrono>

#include "Gl/Shader.hpp"
#include "Sky/SkySystem.hpp"
#include "Sky/Clouds/CloudsModel.hpp"
#include "Sky/Weather.hpp"

#include <iostream>

Camera camera;
float exposure = 0.0005f;
float gamma = 2.2f;
float speed = 100.f / Sky::LenghtUnitInMeters;

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
    .luminance = Sky::Atm::Luminance::Approximate,
    .numScatteringOrders = 4,
};

Sky::Clouds::CloudsParameters cloudsParams = {
    .maxDistance = 128000.f,
    .sigmaS = 0.01f,
    .sigmaA = 0.0f,

    .cloudLayerThickness = Sky::Clouds::CLOUD_LAYER_THICKNESS,
    .cloudLayerBottom = Sky::Clouds::CLOUD_LAYER_BOTTOM,
    .highCloudsHeight = Sky::Clouds::HIGH_CLOUDS_HEIGHT,

    .highCloudsScale = 32 * 1000.0,
    .weatherMapScale = 128 * 1000.0,
    .baseNoiseScale = 25 * 1000.0,
    .detailNoiseScale = 1.5 * 1000.0,
};

Sky::SkyShaders skyShaders;
Sky::SkySystem sky;

constexpr int render_width = 1600;
constexpr int render_height = 900;
float daytime = 0;

void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(Gl::window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void DrawMetrics(double dt) {
    ImGui::Begin("Metrics");

    ImGui::Text("CPU Frametime: %.2f", dt * 1000.f);
    ImGui::Text("FPS: %.2f", 1 / dt);
    ImGui::Text("Pos: %.2f, %.2f, %.2f", camera.position.x, camera.position.y, camera.position.z);
    ImGui::Text("Speed: %.1f", speed);

    ImGui::End();
}

void DrawSettigs() {
    ImGui::Begin("Settings");

    if (ImGui::Button("Clear")) sky.setWeather(Sky::WeatherType::Clear, 5);
    ImGui::SameLine();
    if (ImGui::Button("ScatteredClouds")) sky.setWeather(Sky::WeatherType::Scattered, 5);
    ImGui::SameLine();
    if (ImGui::Button("BrokenClouds")) sky.setWeather(Sky::WeatherType::Broken, 5);
    ImGui::SameLine();
    if (ImGui::Button("Overcast")) sky.setWeather(Sky::WeatherType::Overcast, 5);
    ImGui::SameLine();
    if (ImGui::Button("Storm")) sky.setWeather(Sky::WeatherType::Storm, 5);

    ImGui::InputFloat("Gamma", &gamma);
    ImGui::InputFloat("Sens", &camera.sensitivity);
    if (ImGui::SliderFloat("FOV", &camera.fov, 55, 100, "%.1f")) camera.calcProjMat(render_width, render_height);
    ImGui::InputFloat("Exposure", &exposure, 0.00001, 0.0001, "%.5f");

    ImGui::Text("Skip time:");
    ImGui::SameLine();
    if (ImGui::Button("30min")) sky.addTime(1800);
    ImGui::SameLine();
    if (ImGui::Button("1h")) sky.addTime(3600);
    ImGui::SameLine();
    if (ImGui::Button("3h")) sky.addTime(10800);
    ImGui::SameLine();
    if (ImGui::Button("6h")) sky.addTime(21600);

    ImGui::Text("Set time:");
    ImGui::SameLine();
    if (ImGui::Button("6AM")) sky.setTime(21600);
    ImGui::SameLine();
    if (ImGui::Button("12AM")) sky.setTime(43200);
    ImGui::SameLine();
    if (ImGui::Button("6PM")) sky.setTime(64800);

    if (ImGui::CollapsingHeader("CloudsParameters")) {
        ImGui::InputFloat("SigmaScattering", &cloudsParams.sigmaS, 0.001, 0.01);
        ImGui::InputFloat("SigmaAbsorption", &cloudsParams.sigmaA, 0.001, 0.01);
        ImGui::InputFloat("BaseNoiseScale", &cloudsParams.baseNoiseScale);
        ImGui::InputFloat("DetailNoiseScale", &cloudsParams.detailNoiseScale);

        ImGui::InputFloat("CloudLayerThickness", &cloudsParams.cloudLayerThickness);
        ImGui::InputFloat("CloudLayerBottom", &cloudsParams.cloudLayerBottom);
        ImGui::InputFloat("HighCloudsHeight", &cloudsParams.highCloudsHeight);

        if (ImGui::Button("Update")) sky.setCloudsParameters(cloudsParams);
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

        if (ImGui::Button("Compute model")) sky.recomputeAtmosphere(atm_params);
    }

    ImGui::End();
}

void DrawSkyDebugInfo() {
    ImGui::Begin("SkyDebugInfo");

    const Sky::SkyDebugInfo debugInfo = sky.getDebugInfo();

    const float t = fmod(debugInfo.dayTime, Sky::DAY_LENGHT);
    const int hour = static_cast<int>(t / 3600.0f);
    const int minute = static_cast<int>((t - hour * 3600.0f) / 60.0f);

    std::string formatted = std::format("dayTime: {} ~ {}:{} \nsunDirection: {:.3} {:.3} {:.3}\nwindSpeed: "
                                        "{}\ntransitionDuration: {}\nisTransitioning: {}\nblendFactor: {}",
        debugInfo.dayTime, hour, minute, debugInfo.sunDirection.x, debugInfo.sunDirection.y, debugInfo.sunDirection.z,
        debugInfo.windSpeed * Sky::LenghtUnitInMeters, debugInfo.transitionDuration, debugInfo.isTransitioning, debugInfo.blendFactor);
    ImGui::Text(formatted.c_str());

    ImGui::Image((ImTextureID)(intptr_t)debugInfo.currWeatherMapHandle, ImVec2(256, 256));
    ImGui::Image((ImTextureID)(intptr_t)debugInfo.nextWeatherMapHandle, ImVec2(256, 256));

    ImGui::End();
}

void HotReload() {
    PTR_SAFE_DELETE(skyShaders.atmosphereShader);
    PTR_SAFE_DELETE(skyShaders.cloudsShader);
    PTR_SAFE_DELETE(skyShaders.composeShader);

    skyShaders.atmosphereShader = new Gl::Shader("shaders/ray.vert", "shaders/sky_pass.frag");
    skyShaders.cloudsShader = new Gl::Shader("shaders/ray.vert", "shaders/clouds_pass.frag");
    skyShaders.composeShader = new Gl::Shader("shaders/compose.vert", "shaders/compose.frag");

    sky.setShaders(skyShaders);
}

void InitialLoadShaders() {
    skyShaders.clear2DShader = new Gl::ComputeShader("shaders/atmosphere/clear_2d_cs.glsl");
    skyShaders.clear3DShader = new Gl::ComputeShader("shaders/atmosphere/clear_3d_cs.glsl");
    skyShaders.transmittanceShader = new Gl::ComputeShader("shaders/atmosphere/compute_transmittance_cs.glsl");
    skyShaders.directIrradianceShader = new Gl::ComputeShader("shaders/atmosphere/compute_direct_irradiance_cs.glsl");
    skyShaders.indirectIrradianceShader = new Gl::ComputeShader("shaders/atmosphere/compute_indirect_irradiance_cs.glsl");
    skyShaders.multipleScatteringShader = new Gl::ComputeShader("shaders/atmosphere/compute_multiple_scattering_cs.glsl");
    skyShaders.scatteringDensityShader = new Gl::ComputeShader("shaders/atmosphere/compute_scattering_density_cs.glsl");
    skyShaders.singleScatteringShader = new Gl::ComputeShader("shaders/atmosphere/compute_single_scattering_cs.glsl");

    skyShaders.atmosphereShader = new Gl::Shader("shaders/ray.vert", "shaders/sky_pass.frag");
    skyShaders.cloudsShader = new Gl::Shader("shaders/ray.vert", "shaders/clouds_pass.frag");
    skyShaders.composeShader = new Gl::Shader("shaders/compose.vert", "shaders/compose.frag");

    skyShaders.baseNoiseShader = new Gl::ComputeShader("shaders/clouds/base_noise.comp");
    skyShaders.detailNoiseShader = new Gl::ComputeShader("shaders/clouds/detail_noise.comp");
}

void ClearPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ProcessKeys(double dt) {
    if (Input::IsKeyPressed(Gl::Key::GraveAccent)) Input::ToggleCursor();
    if (Input::IsKeyPressed(Gl::Key::R)) HotReload();

    // presets

    if (Input::IsKeyDown(Gl::Key::PageUp)) speed += 100 / Sky::LenghtUnitInMeters;
    if (Input::IsKeyDown(Gl::Key::PageDown)) speed -= 100 / Sky::LenghtUnitInMeters;

    float speeddt = dt * speed;
    if (Input::IsKeyDown(Gl::Key::LeftShift)) speeddt *= 10;
    if (Input::IsKeyDown(Gl::Key::RightShift)) speeddt *= 100;

    if (Input::IsKeyDown(Gl::Key::Space)) camera.position += camera.up * speeddt;
    if (Input::IsKeyDown(Gl::Key::LeftControl)) camera.position -= camera.up * speeddt;

    // WASD movement
    if (Input::IsKeyDown(Gl::Key::W)) camera.position += camera.forward * speeddt;
    if (Input::IsKeyDown(Gl::Key::S)) camera.position -= camera.forward * speeddt;
    glm::vec3 right = glm::normalize(glm::cross(camera.forward, camera.up));
    if (Input::IsKeyDown(Gl::Key::D)) camera.position += right * speeddt;
    if (Input::IsKeyDown(Gl::Key::A)) camera.position -= right * speeddt;
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

uint8_t* LoadHighCloudsMap() {
    stbi_set_flip_vertically_on_load(true);

    int channels, x, y;
    unsigned char* cirrus = stbi_load("./res/cirrus.png", &x, &y, &channels, 0);
    std::cout << std::format("cirrus.png is loaded ({0}x{1}x{2})", x, y, channels) << std::endl;
    assert(cirrus);
    unsigned char* alto = stbi_load("./res/alto.png", &x, &y, &channels, 0);
    std::cout << std::format("alto.png is loaded ({0}x{1}x{2})", x, y, channels) << std::endl;
    assert(alto);

    constexpr int res = Sky::Clouds::HIGH_CLOUDS_MAP_SIZE * Sky::Clouds::HIGH_CLOUDS_MAP_SIZE;
    auto* combined = new uint8_t[res * 2];
    for (int i = 0; i < res; i++) {
        combined[i * 2 + 0] = cirrus[i]; // R = cirrus
        combined[i * 2 + 1] = alto[i];   // G = alto
    }

    stbi_image_free(cirrus);
    stbi_image_free(alto);

    return combined;
}

int main() {
    Gl::Init();
    Input::SetWindowSize(render_width, render_height);
    InitImGui();

    glfwSwapInterval(1); // vsync

    LoadIcon("res/icon.png");

    Input::Init();
    Input::SetCursorVisible(false);

    InitialLoadShaders();

    uint8_t* highCloudsMap = LoadHighCloudsMap();
    sky.initialize(cloudsParams, atm_params, skyShaders, highCloudsMap);
    delete highCloudsMap;
    sky.setWeather(Sky::WeatherType::Overcast, 5);

    camera = Camera(0.1, 55, {0, 0, 0});
    camera.calcProjMat(render_width, render_height);

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

        ProcessKeys(dt);

        sky.update(dt);

        camera.calcViewMat();
        camera.calcForwardMat();

        if (!Input::IsCursorVisible()) camera.updateControls(dt);
        // -------------- render --------------

        sky.render(camera, 0, gamma, exposure);
        // imgui
        DrawSkyDebugInfo();
        DrawMetrics(dt);
        DrawSettigs();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(Gl::window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
