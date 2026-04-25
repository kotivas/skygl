#include <glad/glad.h>
#include "AtmosphereModel.hpp"
#include "Utils.hpp"
#include "Gl/GlUtils.hpp"


#include <iostream>

namespace Sky::Atm {
    static double kDefaultLambdas[] = {LAMBDA_RED, LAMBDA_GREEN, LAMBDA_BLUE};
    static double kDefaultLuminanceFromRadiance[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

    void swap(Gl::Texture** arr) {
        Gl::Texture* tmp = arr[READ];
        arr[READ] = arr[WRITE];
        arr[WRITE] = tmp;
    }

    void swap(Gl::Texture3D** arr) {
        Gl::Texture3D* tmp = arr[READ];
        arr[READ] = arr[WRITE];
        arr[WRITE] = tmp;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    AtmosphereModel::AtmosphereModel() :
        _rayleighDensity(nullptr), _mieDensity(nullptr), _miePhaseFunctionG(0), _maxSunZenithAngle(0), _luminance(), _halfPrecision(false),
        _textureBuffer(nullptr) {}

    // -----------------------------------------------------------------------------------------------------------------------------------

    AtmosphereModel::~AtmosphereModel() {
        for (auto layer : _absorptionDensity) PTR_SAFE_DELETE(layer);

        _absorptionDensity.clear();

        PTR_SAFE_DELETE(_textureBuffer);

        PTR_SAFE_DELETE(_mieDensity);
        PTR_SAFE_DELETE(_rayleighDensity);

        PTR_SAFE_DELETE(clear2DShader);
        PTR_SAFE_DELETE(clear3DShader);
        PTR_SAFE_DELETE(transmittanceShader);
        PTR_SAFE_DELETE(directIrradianceShader);
        PTR_SAFE_DELETE(indirectIrradianceShader);
        PTR_SAFE_DELETE(multipleScatteringShader);
        PTR_SAFE_DELETE(scatteringDensityShader);
        PTR_SAFE_DELETE(singleScatteringShader);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::initialize(const AtmosphereParameters& params) {
        // clear prev data
        _wavelengths.clear();
        _solarIrradiance.clear();
        _rayleighScattering.clear();
        _mieScattering.clear();
        _mieExtinction.clear();
        _absorptionExtinction.clear();
        _groundAlbedo.clear();

        PTR_SAFE_DELETE(_rayleighDensity);
        PTR_SAFE_DELETE(_mieDensity);
        for (auto layer : _absorptionDensity) PTR_SAFE_DELETE(layer);
        _absorptionDensity.clear();

        // == new

        _luminance = params.luminance;
        _miePhaseFunctionG = params.miePhaseFunctionG;

        _halfPrecision = false;
        _maxSunZenithAngle = glm::radians(120.0);

        _rayleighDensity = new DensityProfileLayer("rayleigh", 0.0, 1.0, -1.0 / params.rayleighScaleHeight, 0.0, 0.0);
        _mieDensity = new DensityProfileLayer("mie", 0.0, 1.0, -1.0 / params.mieScaleHeight, 0.0, 0.0);

        // Density profile increasing linearly from 0 to 1 between 10 and 25km, and
        // decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
        // profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
        // Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
        _absorptionDensity.push_back(new DensityProfileLayer("absorption0", 25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
        _absorptionDensity.push_back(new DensityProfileLayer("absorption1", 0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

        // ====== through wavelenghts (360-840) with 10 nm step ======
        for (int l = LAMBDA_MIN; l <= LAMBDA_MAX; l += 10) {
            double lambda = l * 1e-3; // micro-meters
            double mie = params.mieAngstromBeta / params.mieScaleHeight * pow(lambda, -params.mieAngstromAlpha);

            _wavelengths.push_back(l);

            _solarIrradiance.push_back(EARTH_SOLAR_IRRADIANCE[(l - (int)LAMBDA_MIN) / 10]);
            _rayleighScattering.push_back(params.rayleigh * pow(lambda, -4));
            _mieScattering.push_back(mie * params.mieSingleScatteringAlbedo);
            _mieExtinction.push_back(mie);
            _absorptionExtinction.push_back(params.maxOzoneNumberDensity * EARTH_OZONE_CROSS_SECTION[(l - (int)LAMBDA_MIN) / 10]); // 0 for no ozone
            _groundAlbedo.push_back(params.groundAlbedo);
        }

        // ====== precompute ======

        auto* buffer = new TextureBuffer(_halfPrecision);
        buffer->clear(clear2DShader, clear3DShader);

        // The actual precomputations depend on whether we want to store precomputed
        // irradiance or illuminance values.
        if (num_precomputed_wavelengths() <= 3) {
            precompute(buffer, nullptr, nullptr, false, params.numScatteringOrders);
        } else {
            int num_iterations = (num_precomputed_wavelengths() + 2) / 3;
            double dlambda = (LAMBDA_MAX - LAMBDA_MIN) / (3.0 * num_iterations);

            for (int i = 0; i < num_iterations; ++i) {
                double lambdas[] = {LAMBDA_MIN + (3 * i + 0.5) * dlambda, LAMBDA_MIN + (3 * i + 1.5) * dlambda, LAMBDA_MIN + (3 * i + 2.5) * dlambda};

                double luminance_from_radiance[] = {coeff(lambdas[0], 0) * dlambda, coeff(lambdas[1], 0) * dlambda, coeff(lambdas[2], 0) * dlambda,
                    coeff(lambdas[0], 1) * dlambda, coeff(lambdas[1], 1) * dlambda, coeff(lambdas[2], 1) * dlambda, coeff(lambdas[0], 2) * dlambda,
                    coeff(lambdas[1], 2) * dlambda, coeff(lambdas[2], 2) * dlambda};

                bool blend = i > 0;
                precompute(buffer, lambdas, luminance_from_radiance, blend, params.numScatteringOrders);
            }

            // After the above iterations, the transmittance texture contains the
            // transmittance for the 3 wavelengths used at the last iteration. But we
            // want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
            // must recompute it here for these 3 wavelengths:
            transmittanceShader->use();

            bind_compute_uniforms(transmittanceShader, nullptr, nullptr);
            // buffer->transmittance_array[WRITE]->bind_image(0, 0, GL_READ_WRITE, GL_RGBA32F);
            buffer->transmittance_array[WRITE]->bindImage(0, GL_READ_WRITE);
            transmittanceShader->setUniform4f("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

            transmittanceShader->dispatch(TRANSMITTANCE_WIDTH / NUM_THREADS, TRANSMITTANCE_HEIGHT / NUM_THREADS, 1);
            transmittanceShader->barrier();

            swap(buffer->transmittance_array);
        }

        PTR_SAFE_DELETE(_textureBuffer)
        _textureBuffer = buffer;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::bind_uniform(Gl::Shader* shader) {
        shader->setUniform1i("transmittance_texture", 0);
        _textureBuffer->transmittance_array[READ]->bind(0);

        shader->setUniform1i("scattering_texture", 1);
        _textureBuffer->scattering_array[READ]->bind(1);

        shader->setUniform1i("irradiance_texture", 2);
        _textureBuffer->irradiance_array[READ]->bind(2);

        shader->setUniform1i("TRANSMITTANCE_TEXTURE_WIDTH", TRANSMITTANCE_WIDTH);
        shader->setUniform1i("TRANSMITTANCE_TEXTURE_HEIGHT", TRANSMITTANCE_HEIGHT);
        shader->setUniform1i("SCATTERING_TEXTURE_R_SIZE", SCATTERING_R);
        shader->setUniform1i("SCATTERING_TEXTURE_MU_SIZE", SCATTERING_MU);
        shader->setUniform1i("SCATTERING_TEXTURE_MU_S_SIZE", SCATTERING_MU_S);
        shader->setUniform1i("SCATTERING_TEXTURE_NU_SIZE", SCATTERING_NU);
        shader->setUniform1i("SCATTERING_TEXTURE_WIDTH", SCATTERING_WIDTH);
        shader->setUniform1i("SCATTERING_TEXTURE_HEIGHT", SCATTERING_HEIGHT);
        shader->setUniform1i("SCATTERING_TEXTURE_DEPTH", SCATTERING_DEPTH);
        shader->setUniform1i("IRRADIANCE_TEXTURE_WIDTH", IRRADIANCE_WIDTH);
        shader->setUniform1i("IRRADIANCE_TEXTURE_HEIGHT", IRRADIANCE_HEIGHT);

        shader->setUniform1f("sun_angular_radius", SUN_ANGULAR_RADIUS);
        shader->setUniform1f("bottom_radius", EARTH_BOTTOM_RADIUS / LenghtUnitInMeters);
        shader->setUniform1f("top_radius", EARTH_TOP_RADIUS / LenghtUnitInMeters);
        shader->setUniform1f("mie_phase_function_g", (float)_miePhaseFunctionG);
        shader->setUniform1f("mu_s_min", (float)cos(_maxSunZenithAngle));

        glm::vec3 sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance;
        sky_sun_radiance_to_luminance(sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance);

        shader->setUniform3f("SKY_SPECTRAL_RADIANCE_TO_LUMINANCE", sky_spectral_radiance_to_luminance);
        shader->setUniform3f("SUN_SPECTRAL_RADIANCE_TO_LUMINANCE", sun_spectral_radiance_to_luminance);

        glm::vec3 solar_irradiance_ = to_vector(_wavelengths, _solarIrradiance, kDefaultLambdas, 1.0);
        shader->setUniform3f("solar_irradiance", solar_irradiance_);

        glm::vec3 rayleigh_scattering_ = to_vector(_wavelengths, _rayleighScattering, kDefaultLambdas, LenghtUnitInMeters);
        shader->setUniform3f("rayleigh_scattering", rayleigh_scattering_);

        glm::vec3 mie_scattering_ = to_vector(_wavelengths, _mieScattering, kDefaultLambdas, LenghtUnitInMeters);
        shader->setUniform3f("mie_scattering", mie_scattering_);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::convert_spectrum_to_linear_srgb(double& r, double& g, double& b) const {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        const int dlambda = 1;
        for (int lambda = LAMBDA_MIN; lambda < LAMBDA_MAX; lambda += dlambda) {
            double value = interpolate(_wavelengths, _solarIrradiance, lambda);
            x += cie_color_matching_function_table_value(lambda, 1) * value;
            y += cie_color_matching_function_table_value(lambda, 2) * value;
            z += cie_color_matching_function_table_value(lambda, 3) * value;
        }

        const double* XYZ_TO_SRGB = &kXYZ_TO_SRGB[0];
        r = static_cast<double>(MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
        g = static_cast<double>(MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
        b = static_cast<double>(MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    double AtmosphereModel::coeff(double lambda, int component) {
        // Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
        // artefacts due to too large values when using half precision on GPU.
        // We add this term back in kAtmosphereShader, via
        // SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
        // Model constructor).
        double x = cie_color_matching_function_table_value(lambda, 1);
        double y = cie_color_matching_function_table_value(lambda, 2);
        double z = cie_color_matching_function_table_value(lambda, 3);
        double sRGB = kXYZ_TO_SRGB[component * 3 + 0] * x + kXYZ_TO_SRGB[component * 3 + 1] * y + kXYZ_TO_SRGB[component * 3 + 2] * z;

        return sRGB;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::bind_compute_uniforms(Gl::ComputeShader* shader, double* lambdas, double* luminance_from_radiance) {
        if (lambdas == nullptr) lambdas = kDefaultLambdas;

        if (luminance_from_radiance == nullptr) luminance_from_radiance = kDefaultLuminanceFromRadiance;

        shader->setUniform1i("TRANSMITTANCE_TEXTURE_WIDTH", TRANSMITTANCE_WIDTH);
        shader->setUniform1i("TRANSMITTANCE_TEXTURE_HEIGHT", TRANSMITTANCE_HEIGHT);
        shader->setUniform1i("SCATTERING_TEXTURE_R_SIZE", SCATTERING_R);
        shader->setUniform1i("SCATTERING_TEXTURE_MU_SIZE", SCATTERING_MU);
        shader->setUniform1i("SCATTERING_TEXTURE_MU_S_SIZE", SCATTERING_MU_S);
        shader->setUniform1i("SCATTERING_TEXTURE_NU_SIZE", SCATTERING_NU);
        shader->setUniform1i("SCATTERING_TEXTURE_WIDTH", SCATTERING_WIDTH);
        shader->setUniform1i("SCATTERING_TEXTURE_HEIGHT", SCATTERING_HEIGHT);
        shader->setUniform1i("SCATTERING_TEXTURE_DEPTH", SCATTERING_DEPTH);
        shader->setUniform1i("IRRADIANCE_TEXTURE_WIDTH", IRRADIANCE_WIDTH);
        shader->setUniform1i("IRRADIANCE_TEXTURE_HEIGHT", IRRADIANCE_HEIGHT);

        glm::vec3 sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance;
        sky_sun_radiance_to_luminance(sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance);

        shader->setUniform3f("SKY_SPECTRAL_RADIANCE_TO_LUMINANCE", sky_spectral_radiance_to_luminance);
        shader->setUniform3f("SUN_SPECTRAL_RADIANCE_TO_LUMINANCE", sun_spectral_radiance_to_luminance);

        glm::vec3 solar_irradiance_ = to_vector(_wavelengths, _solarIrradiance, lambdas, 1.0);
        shader->setUniform3f("solar_irradiance", solar_irradiance_);

        glm::vec3 rayleigh_scattering_ = to_vector(_wavelengths, _rayleighScattering, lambdas, LenghtUnitInMeters);
        bind_density_layer(shader, _rayleighDensity);
        shader->setUniform3f("rayleigh_scattering", rayleigh_scattering_);

        glm::vec3 mie_scattering_ = to_vector(_wavelengths, _mieScattering, lambdas, LenghtUnitInMeters);
        glm::vec3 mie_extinction_ = to_vector(_wavelengths, _mieExtinction, lambdas, LenghtUnitInMeters);
        bind_density_layer(shader, _mieDensity);
        shader->setUniform3f("mie_scattering", mie_scattering_);
        shader->setUniform3f("mie_extinction", mie_extinction_);

        glm::vec3 absorption_extinction_ = to_vector(_wavelengths, _absorptionExtinction, lambdas, LenghtUnitInMeters);
        bind_density_layer(shader, _absorptionDensity[0]);
        bind_density_layer(shader, _absorptionDensity[1]);
        shader->setUniform3f("absorption_extinction", absorption_extinction_);

        glm::vec3 groundAlbedo = to_vector(_wavelengths, _groundAlbedo, lambdas, 1.0);
        shader->setUniform3f("ground_albedo", groundAlbedo);

        shader->setUniformMat4fv("luminanceFromRadiance", to_matrix(luminance_from_radiance));
        shader->setUniform1f("sun_angular_radius", SUN_ANGULAR_RADIUS);
        shader->setUniform1f("bottom_radius", EARTH_BOTTOM_RADIUS / LenghtUnitInMeters);
        shader->setUniform1f("top_radius", EARTH_TOP_RADIUS / LenghtUnitInMeters);
        shader->setUniform1f("mie_phase_function_g", (float)_miePhaseFunctionG);
        shader->setUniform1f("mu_s_min", (float)cos(_maxSunZenithAngle));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::bind_density_layer(Gl::ComputeShader* shader, DensityProfileLayer* layer) const {
        shader->setUniform1f(layer->name + "_width", (float)(layer->width / LenghtUnitInMeters));
        shader->setUniform1f(layer->name + "_exp_term", (float)layer->exp_term);
        shader->setUniform1f(layer->name + "_exp_scale", (float)(layer->exp_scale * LenghtUnitInMeters));
        shader->setUniform1f(layer->name + "_linear_term", (float)(layer->linear_term * LenghtUnitInMeters));
        shader->setUniform1f(layer->name + "_constant_term", (float)layer->constant_term);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::sky_sun_radiance_to_luminance(glm::vec3& sky_spectral_radiance_to_luminance, glm::vec3& sun_spectral_radiance_to_luminance) const {
        bool precompute_illuminance = num_precomputed_wavelengths() > 3;
        double sky_k_r, sky_k_g, sky_k_b;

        if (precompute_illuminance) sky_k_r = sky_k_g = sky_k_b = static_cast<double>(MAX_LUMINOUS_EFFICACY);
        else compute_spectral_radiance_to_luminance_factors(_wavelengths, _solarIrradiance, -3, sky_k_r, sky_k_g, sky_k_b);

        // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
        double sun_k_r, sun_k_g, sun_k_b;
        compute_spectral_radiance_to_luminance_factors(_wavelengths, _solarIrradiance, 0, sun_k_r, sun_k_g, sun_k_b);

        sky_spectral_radiance_to_luminance = glm::vec3((float)sky_k_r, (float)sky_k_g, (float)sky_k_b);
        sun_spectral_radiance_to_luminance = glm::vec3((float)sun_k_r, (float)sun_k_g, (float)sun_k_b);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::precompute(TextureBuffer* buffer, double* lambdas, double* luminance_from_radiance, bool blend, int num_scattering_orders) {
        int BLEND = blend ? 1 : 0;

        // ------------------------------------------------------------------
        // Compute Transmittance
        // ------------------------------------------------------------------

        transmittanceShader->use();
        bind_compute_uniforms(transmittanceShader, lambdas, luminance_from_radiance);

        buffer->transmittance_array[WRITE]->bindImage(0, GL_READ_WRITE);

        transmittanceShader->setUniform4f("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        transmittanceShader->dispatch(TRANSMITTANCE_WIDTH / NUM_THREADS, TRANSMITTANCE_HEIGHT / NUM_THREADS, 1);
        transmittanceShader->barrier();

        swap(buffer->transmittance_array);

        // ------------------------------------------------------------------
        // Compute Direct Irradiance
        // ------------------------------------------------------------------

        directIrradianceShader->use();
        bind_compute_uniforms(directIrradianceShader, lambdas, luminance_from_radiance);

        buffer->irradiance_array[READ]->bindImage(0, GL_READ_WRITE);
        buffer->irradiance_array[WRITE]->bindImage(1, GL_READ_WRITE);
        buffer->delta_irradiance_texture->bindImage(2, GL_READ_WRITE);

        directIrradianceShader->setUniform1i("transmittance", 3);
        buffer->transmittance_array[READ]->bind(3);

        directIrradianceShader->setUniform4f("blend", glm::vec4(0.0f, BLEND, 0.0f, 0.0f));
        directIrradianceShader->dispatch(IRRADIANCE_WIDTH / NUM_THREADS, IRRADIANCE_HEIGHT / NUM_THREADS, 1);
        directIrradianceShader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        swap(buffer->irradiance_array);

        // ------------------------------------------------------------------
        // Compute Single Scattering
        // ------------------------------------------------------------------

        singleScatteringShader->use();
        bind_compute_uniforms(singleScatteringShader, lambdas, luminance_from_radiance);

        buffer->delta_rayleigh_scattering_texture->bindImage(0, GL_READ_WRITE);
        buffer->delta_mie_scattering_texture->bindImage(1, GL_READ_WRITE);
        buffer->scattering_array[READ]->bindImage(2, GL_READ_WRITE);
        buffer->scattering_array[WRITE]->bindImage(3, GL_READ_WRITE);

        singleScatteringShader->setUniform1i("transmittance", 6);
        buffer->transmittance_array[READ]->bind(6);

        singleScatteringShader->setUniform4f("blend", glm::vec4(0.0f, 0.0f, BLEND, BLEND));

        for (int layer = 0; layer < SCATTERING_DEPTH; ++layer) {
            singleScatteringShader->setUniform1i("layer", layer);
            singleScatteringShader->dispatch(SCATTERING_WIDTH / NUM_THREADS, SCATTERING_HEIGHT / NUM_THREADS, 1);
            singleScatteringShader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        swap(buffer->scattering_array);

        for (int scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order) {
            // ------------------------------------------------------------------
            // Compute Scattering Density
            // ------------------------------------------------------------------

            scatteringDensityShader->use();
            bind_compute_uniforms(scatteringDensityShader, lambdas, luminance_from_radiance);

            buffer->delta_scattering_density_texture->bindImage(0, GL_READ_WRITE);

            scatteringDensityShader->setUniform1i("transmittance", 1);
            buffer->transmittance_array[READ]->bind(1);

            scatteringDensityShader->setUniform1i("single_rayleigh_scattering", 2);
            buffer->delta_rayleigh_scattering_texture->bind(2);

            scatteringDensityShader->setUniform1i("single_mie_scattering", 3);
            buffer->delta_mie_scattering_texture->bind(3);

            scatteringDensityShader->setUniform1i("multiple_scattering", 4);
            buffer->delta_multiple_scattering_texture->bind(4);

            scatteringDensityShader->setUniform1i("irradiance", 5);
            buffer->delta_irradiance_texture->bind(5);

            scatteringDensityShader->setUniform1i("scattering_order", scattering_order);
            scatteringDensityShader->setUniform4f("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

            for (int layer = 0; layer < SCATTERING_DEPTH; ++layer) {
                scatteringDensityShader->setUniform1i("layer", layer);
                scatteringDensityShader->dispatch(SCATTERING_WIDTH / NUM_THREADS, SCATTERING_HEIGHT / NUM_THREADS, 1);
                scatteringDensityShader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            }

            // ------------------------------------------------------------------
            // Compute Indirect Irradiance
            // ------------------------------------------------------------------

            indirectIrradianceShader->use();
            bind_compute_uniforms(indirectIrradianceShader, lambdas, luminance_from_radiance);

            buffer->delta_irradiance_texture->bindImage(0, GL_READ_WRITE);
            buffer->irradiance_array[READ]->bindImage(1, GL_READ_WRITE);
            buffer->irradiance_array[WRITE]->bindImage(2, GL_READ_WRITE);

            indirectIrradianceShader->setUniform1i("single_rayleigh_scattering", 3);
            buffer->delta_rayleigh_scattering_texture->bind(3);

            indirectIrradianceShader->setUniform1i("single_mie_scattering", 4);
            buffer->delta_mie_scattering_texture->bind(4);

            indirectIrradianceShader->setUniform1i("multiple_scattering", 5);
            buffer->delta_multiple_scattering_texture->bind(5);

            indirectIrradianceShader->setUniform1i("scattering_order", scattering_order - 1);
            indirectIrradianceShader->setUniform4f("blend", glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

            indirectIrradianceShader->dispatch(IRRADIANCE_WIDTH / NUM_THREADS, IRRADIANCE_HEIGHT / NUM_THREADS, 1);
            indirectIrradianceShader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            swap(buffer->irradiance_array);

            // ------------------------------------------------------------------
            // Compute Multiple Scattering
            // ------------------------------------------------------------------

            multipleScatteringShader->use();
            bind_compute_uniforms(multipleScatteringShader, lambdas, luminance_from_radiance);

            buffer->delta_multiple_scattering_texture->bindImage(0, GL_READ_WRITE);
            buffer->scattering_array[READ]->bindImage(1, GL_READ_WRITE);
            buffer->scattering_array[WRITE]->bindImage(2, GL_READ_WRITE);

            multipleScatteringShader->setUniform1i("transmittance", 3);
            buffer->transmittance_array[READ]->bind(3);

            multipleScatteringShader->setUniform1i("delta_scattering_density", 4);
            buffer->delta_scattering_density_texture->bind(4);

            multipleScatteringShader->setUniform4f("blend", glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

            for (int layer = 0; layer < SCATTERING_DEPTH; ++layer) {
                multipleScatteringShader->setUniform1i("layer", layer);
                indirectIrradianceShader->dispatch(SCATTERING_WIDTH / NUM_THREADS, SCATTERING_HEIGHT / NUM_THREADS, 1);
                indirectIrradianceShader->barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            }

            swap(buffer->scattering_array);
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------------------------------------------------------

    glm::vec3 AtmosphereModel::to_vector(const std::vector<double>& wavelengths, const std::vector<double>& v, double lambdas[], double scale) {
        double r = interpolate(wavelengths, v, lambdas[0]) * scale;
        double g = interpolate(wavelengths, v, lambdas[1]) * scale;
        double b = interpolate(wavelengths, v, lambdas[2]) * scale;

        return {(float)r, (float)g, (float)b};
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    glm::mat4 AtmosphereModel::to_matrix(double arr[]) {
        return {(float)arr[0], (float)arr[3], (float)arr[6], 0.0f, (float)arr[1], (float)arr[4], (float)arr[7], 0.0f, (float)arr[2], (float)arr[5],
            (float)arr[8], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    double AtmosphereModel::cie_color_matching_function_table_value(double wavelength, int column) {
        if (wavelength <= LAMBDA_MIN || wavelength >= LAMBDA_MAX) return 0.0;

        double u = (wavelength - LAMBDA_MIN) / 5.0;
        int row = static_cast<int>(floor(u));

        u -= row;
        return kCIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) + kCIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    double AtmosphereModel::interpolate(const std::vector<double>& wavelengths, const std::vector<double>& wavelength_function, double wavelength) {
        if (wavelength < wavelengths[0]) return wavelength_function[0];

        for (int i = 0; i < wavelengths.size() - 1; ++i) {
            if (wavelength < wavelengths[i + 1]) {
                double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
                return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
            }
        }

        return wavelength_function[wavelength_function.size() - 1];
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void AtmosphereModel::compute_spectral_radiance_to_luminance_factors(
        const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance, double lambda_power, double& k_r, double& k_g, double& k_b) {
        k_r = 0.0;
        k_g = 0.0;
        k_b = 0.0;
        double solar_r = interpolate(wavelengths, solar_irradiance, LAMBDA_RED);
        double solar_g = interpolate(wavelengths, solar_irradiance, LAMBDA_GREEN);
        double solar_b = interpolate(wavelengths, solar_irradiance, LAMBDA_BLUE);
        int dlambda = 1;

        for (int lambda = LAMBDA_MIN; lambda < LAMBDA_MAX; lambda += dlambda) {
            double x_bar = cie_color_matching_function_table_value(lambda, 1);
            double y_bar = cie_color_matching_function_table_value(lambda, 2);
            double z_bar = cie_color_matching_function_table_value(lambda, 3);

            const double* xyz2srgb = &kXYZ_TO_SRGB[0];
            double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
            double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
            double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
            double irradiance = interpolate(wavelengths, solar_irradiance, lambda);

            k_r += r_bar * irradiance / solar_r * pow(lambda / LAMBDA_RED, lambda_power);
            k_g += g_bar * irradiance / solar_g * pow(lambda / LAMBDA_GREEN, lambda_power);
            k_b += b_bar * irradiance / solar_b * pow(lambda / LAMBDA_BLUE, lambda_power);
        }

        k_r *= static_cast<double>(MAX_LUMINOUS_EFFICACY) * dlambda;
        k_g *= static_cast<double>(MAX_LUMINOUS_EFFICACY) * dlambda;
        k_b *= static_cast<double>(MAX_LUMINOUS_EFFICACY) * dlambda;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------
} // namespace Sky::Atm
