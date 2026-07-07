#include "SkySystem.hpp"

#include "Input.hpp"
#include "Noise.hpp"
#include "Gl/GlUtils.hpp"
#include "../Utils.hpp"
#include <iostream>

namespace Sky {

SkySystem::SkySystem() :
	_atmosphereShader(nullptr), _cloudsShader(nullptr), _composeShader(nullptr), _atmosphereFBO(0), _atmosphereColor(0), _cloudsFBO(0), _cloudsColor(0),
	_quadVao(0), _quadVbo(0), _dayTime(26000), _rng(std::chrono::high_resolution_clock::now().time_since_epoch().count()) {
}

void SkySystem::initialize(const Clouds::CloudsParameters& clouds_params,
	const Atm::AtmosphereParameters& atmosphere_params,
	const SkyShaders& shaders,
	const uint8_t* highCloudsMap) {

	assert(highCloudsMap);

	setShaders(shaders);

	// --- framebuffers
	Gl::CreateQuadVO(_quadVao, _quadVbo);
	Gl::CreateFrameBuffer(_atmosphereFBO, _atmosphereColor, GL_RGBA32F, GL_RGBA, ATMOSPHERE_FRAME_WIDTH, ATMOSPHERE_FRAME_HEIGHT);
	Gl::CreateFrameBuffer(_cloudsFBO, _cloudsColor, GL_RGBA16F, GL_RGBA, CLOUD_FRAME_WIDTH, CLOUD_FRAME_HEIGHT);
	std::cout << ": Created FBOs (atmosphere, clouds)" << std::endl;

	// --- Atmosphere ---
	std::cout << ": Atmosphere model initialization. This may take a while." << std::endl;
	_atmosphereModel.initialize(atmosphere_params);

	// --- Clouds ---
	std::cout << ": Clouds model initialization." << std::endl;
	_cloudsModel.initialize(clouds_params);
	std::cout << ": Queued cloud base noise computation." << std::endl;
	_cloudsModel.generateBaseNoise(shaders.baseNoiseShader);
	std::cout << ": Queued cloud detail noise computation." << std::endl;
	_cloudsModel.generateDetailNoise(shaders.detailNoiseShader);
	_cloudsModel.setHighCloudsMap(highCloudsMap);

	// --- Weather UBO ---
	_weatherUBO.create(sizeof(WeatherParameters));
	setWeather(WeatherType::Scattered, 1);
}

void SkySystem::atmospherePass(const Camera& camera) {
	assert(_atmosphereShader);
	if (!_atmosphereShader->isValid()) return;

	_atmosphereShader->use();

	_atmosphereShader->setUniform1f("uTime", _dayTime);
	const auto viewNoTrans = glm::mat4(glm::mat3(camera.viewMatrix));
	_atmosphereShader->setUniformMat4fv("uView", viewNoTrans);
	_atmosphereShader->setUniformMat4fv("uProjection", camera.projectionMatrix);
	_atmosphereShader->setUniform3f("uCameraPos", camera.position);

	_atmosphereShader->setUniform3f("uSunDir", _sun.direction);
	_atmosphereShader->setUniform2f("uSunSize", glm::vec2(tan(SUN_ANGULAR_RADIUS), cos(SUN_ANGULAR_RADIUS)));

	_atmosphereModel.bind_uniform(_atmosphereShader);

	glViewport(0, 0, ATMOSPHERE_FRAME_WIDTH, ATMOSPHERE_FRAME_HEIGHT);

	glBindFramebuffer(GL_FRAMEBUFFER, _atmosphereFBO);

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glBindVertexArray(_quadVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void SkySystem::cloudsPass(const Camera& camera) {
	assert(_cloudsShader);
	if (!_cloudsShader->isValid()) return;

	_cloudsShader->use();
	_cloudsShader->setUniform1f("uTime", _dayTime);
	_cloudsShader->setUniformMat4fv("uView", camera.viewMatrix);
	_cloudsShader->setUniformMat4fv("uProjection", camera.projectionMatrix);
	_cloudsShader->setUniform3f("uCameraPos", camera.position);
	_cloudsShader->setUniform3f("uSunDir", _sun.direction);

	_cloudsModel.bind(_cloudsShader);
	_atmosphereModel.bind_uniform(_cloudsShader);
	_weatherUBO.bind(2);

	glViewport(0, 0, CLOUD_FRAME_WIDTH, CLOUD_FRAME_HEIGHT);

	glBindFramebuffer(GL_FRAMEBUFFER, _cloudsFBO);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glBindVertexArray(_quadVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void SkySystem::render(const Camera& camera, uint16_t framebuffer, float gamma, float exposure) {

	atmospherePass(camera);
	cloudsPass(camera);

	// collect all data into the RenderInfo and use it for info, instead of scene

	// ---

	assert(_composeShader);
	if (!_composeShader->isValid()) return;

	_composeShader->use();

	_composeShader->setUniform1f("uGamma", gamma);
	_composeShader->setUniform1f("uExposure", exposure);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _atmosphereColor);
	_composeShader->setUniform1i("skyTexture", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _cloudsColor);
	_composeShader->setUniform1i("cloudsTexture", 1);

	glViewport(0, 0, Input::GetWindowWidth(), Input::GetWindowHeight());

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glBindVertexArray(_quadVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
}

void SkySystem::setShaders(const SkyShaders& shaders) {
	_atmosphereModel.clear2DShader = shaders.clear2DShader;
	_atmosphereModel.clear3DShader = shaders.clear3DShader;
	_atmosphereModel.transmittanceShader = shaders.transmittanceShader;
	_atmosphereModel.directIrradianceShader = shaders.directIrradianceShader;
	_atmosphereModel.indirectIrradianceShader = shaders.indirectIrradianceShader;
	_atmosphereModel.multipleScatteringShader = shaders.multipleScatteringShader;
	_atmosphereModel.scatteringDensityShader = shaders.scatteringDensityShader;
	_atmosphereModel.singleScatteringShader = shaders.singleScatteringShader;

	_atmosphereShader = shaders.atmosphereShader;
	_cloudsShader = shaders.cloudsShader;
	_composeShader = shaders.composeShader;
}

void SkySystem::setTime(float time) {
	_dayTime = time;
}

void SkySystem::addTime(float time) {
	_dayTime += time;
}

float SkySystem::getTime() const {
	return _dayTime;
}

void SkySystem::setCloudsParameters(const Clouds::CloudsParameters& params) {
	_cloudsModel.updateParameters(params);
}

void SkySystem::recomputeAtmosphere(const Atm::AtmosphereParameters& params) {
	_atmosphereModel.initialize(params);
}

WeatherType SkySystem::pickNextWeather() const {
	auto it = WeatherGraph.find(_currentWeather);
	if (it == WeatherGraph.end() || it->second.empty()) return _currentWeather; // нет правил для текущего состояния — остаёмся как есть

	const std::vector<std::pair<WeatherType, float>>& candidates = it->second;

	float totalWeight = 0.0f;
	for (const auto& [type, weight] : candidates) totalWeight += weight;

	std::uniform_real_distribution<float> dist(0.0f, totalWeight);
	float roll = dist(_rng);

	float accumulated = 0.0f;
	for (const auto& [type, weight] : candidates) {
		accumulated += weight;
		if (roll <= accumulated) return type;
	}
	return candidates.back().first; // fallback
}


void SkySystem::setWeather(const WeatherType& preset, float transitionTime) {
	_cloudsModel.swapWeatherMap();

	_currentWeather = preset;

	std::cout << ": Next weather is ";

	WeatherParameters new_params;
	new_params.weatherMapBlend = 0.0f;
	new_params.windSpeed = 2.0f / LenghtUnitInMeters;
	new_params.altoDensity = 0.3f;
	new_params.cirrusDensity = 0.3f;
	std::vector<float> weather;

	switch (preset) {
	case WeatherType::Clear:
		weather = WeatherMapGenerator::GenClearSky();
		std::cout << "(Clear Sky)";
		break;
	case WeatherType::Scattered:
		weather = WeatherMapGenerator::GenScatteredClouds(_dayTime);
		std::cout << "(Scattered Clouds)";
		break;
	case WeatherType::Broken:
		weather = WeatherMapGenerator::GenBrokenClouds(_dayTime);
		std::cout << "(Broken Clouds)";
		break;
	case WeatherType::Overcast:
		weather = WeatherMapGenerator::GenOvercast(_dayTime);
		std::cout << "(Overcast)";
		break;
	case WeatherType::Storm:
		weather = WeatherMapGenerator::GenStorm(_dayTime);
		std::cout << "(Storm)";
		break;
	};

	std::cout << " in " << transitionTime << " second(s)." << std::endl;;
	// std::vector<float> weather = generateWeatherMap(params);
	_cloudsModel.setNextWeatherMap(weather.data());

	_weatherTransition.start(_weatherParameters, new_params, transitionTime);
}

SkyDebugInfo SkySystem::getDebugInfo() const {
	SkyDebugInfo debugInfo;
	debugInfo.dayTime = _dayTime;
	debugInfo.sunDirection = _sun.direction;
	debugInfo.blendFactor = _weatherTransition.current().weatherMapBlend;
	debugInfo.transitionDuration = _weatherTransition.duration;
	debugInfo.isTransitioning = _weatherTransition.running;
	debugInfo.currWeatherMapHandle = _cloudsModel.getCurrentWeatherMap()->getHandle();
	debugInfo.nextWeatherMapHandle = _cloudsModel.getNextWeatherMap()->getHandle();

	return debugInfo;
}

void SkySystem::update(double dt) {
	_dayTime += dt * TIME_MULTIPLIER;

	_weatherTransition.update(dt);
	if (_weatherTransition.running) {
		_weatherParameters = _weatherTransition.current();
		_weatherUBO.setData(&_weatherParameters);
	} else {
		std::uniform_real_distribution<float> dist(120.f, 480.f); // random duration weather time
		setWeather(pickNextWeather(), dist(_rng));
	}

	_sun.update(_dayTime, 80); // 157
}

SkySystem::~SkySystem() {
	glDeleteVertexArrays(1, &_quadVao);
	glDeleteBuffers(1, &_quadVbo);
}


} // namespace Sky
