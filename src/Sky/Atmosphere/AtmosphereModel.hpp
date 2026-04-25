#pragma once

#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Sky/Constants.hpp"
#include "TextureBuffer.hpp"
#include "Gl/Shader.hpp"

namespace Sky::Atm {

#define READ  0
#define WRITE 1

    enum class Luminance {
        None,        // Render the spectral radiance at kLambdaR, kLambdaG, kLambdaB.
        Approximate, // Render the sRGB luminance, using an approximate (on the fly) conversion from 3 spectral
                     // radiance values
        Precomputed  // Render the sRGB luminance, precomputed from 15 spectral radiance values
    };

    struct AtmosphereParameters {
        double maxOzoneNumberDensity;     // Maximum number density of ozone molecules in m^-3.
        double rayleigh;                  // Rayleigh coeff scattering. (^ more constrast)
        double rayleighScaleHeight;       // Height of rayleigh (air density) fallout. (^ thicker atmosphere)
        double mieScaleHeight;            // Height of aerosol fallout (dust, smoke).
        double mieAngstromAlpha;          // Mie dependence on wavelength factor.
        double mieAngstromBeta;           // Mie scattering intensity. (^ sun glare, stronger haze)
        double mieSingleScatteringAlbedo; // The proportion of scattering in total attenuation.
        double miePhaseFunctionG;         // Directionality parameter (Henyey–Greenstein). (^ sun halo)
        double groundAlbedo;              // Reflectivity of the ground.
        Luminance luminance;              // Use radiance or luminance mode.
        int numScatteringOrders;          // ?
    };

    /// An atmosphere layer of width 'width' (in m), and whose density is defined as
    /// 'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
    /// clamped to [0,1], and where h is the altitude (in m). 'exp_term' and
    /// 'constant_term' are unitless, while 'exp_scale' and 'linear_term' are in m^-1.
    struct DensityProfileLayer {
        std::string name;
        double width;
        double exp_term;
        double exp_scale;
        double linear_term;
        double constant_term;

        DensityProfileLayer(const std::string& _name, double _width, double _exp_term, double _exp_scale, double _linear_term, double _constant_term) {
            name = _name;
            width = _width;
            exp_term = _exp_term;
            exp_scale = _exp_scale;
            linear_term = _linear_term;
            constant_term = _constant_term;
        }
    };

    class AtmosphereModel {

    public:
        AtmosphereModel();
        ~AtmosphereModel();

        void initialize(const AtmosphereParameters& params);
        void bind_uniform(Gl::Shader* shader);
        void convert_spectrum_to_linear_srgb(double& r, double& g, double& b) const;

        Gl::ComputeShader* clear2DShader = nullptr;
        Gl::ComputeShader* clear3DShader = nullptr;
        Gl::ComputeShader* transmittanceShader = nullptr;
        Gl::ComputeShader* directIrradianceShader = nullptr;
        Gl::ComputeShader* indirectIrradianceShader = nullptr;
        Gl::ComputeShader* singleScatteringShader = nullptr;
        Gl::ComputeShader* multipleScatteringShader = nullptr;
        Gl::ComputeShader* scatteringDensityShader = nullptr;

    private:
        static double coeff(double lambda, int component);
        void bind_compute_uniforms(Gl::ComputeShader* shader, double* lambdas, double* luminance_from_radiance);
        void bind_density_layer(Gl::ComputeShader* shader, DensityProfileLayer* layer) const;
        void sky_sun_radiance_to_luminance(glm::vec3& sky_spectral_radiance_to_luminance, glm::vec3& sun_spectral_radiance_to_luminance) const;
        void precompute(TextureBuffer* buffer, double* lambdas, double* luminance_from_radiance, bool blend, int num_scattering_orders);

        static glm::vec3 to_vector(const std::vector<double>& wavelengths, const std::vector<double>& v, double lambdas[], double scale);
        static glm::mat4 to_matrix(double arr[]);

        static double cie_color_matching_function_table_value(double wavelength, int column);
        static double interpolate(const std::vector<double>& wavelengths, const std::vector<double>& wavelength_function, double wavelength);
        static void compute_spectral_radiance_to_luminance_factors(
            const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance, double lambda_power, double& k_r, double& k_g, double& k_b);

        // === private members ===

        /// The wavelength values, in nanometers, and sorted in increasing order, for
        /// which the solar_irradiance, rayleigh_scattering, mie_scattering,
        /// mie_extinction and ground_albedo samples are provided. If your shaders
        /// use luminance values (as opposed to radiance values, see above), use a
        /// large number of wavelengths (e.g. between 15 and 50) to get accurate
        /// results (this number of wavelengths has absolutely no impact on the
        /// shader performance).
        std::vector<double> _wavelengths;

        /// The solar irradiance at the top of the atmosphere, in W/m^2/nm. This
        /// vector must have the same size as the wavelengths parameter.
        std::vector<double> _solarIrradiance;

        /// The density profile of air molecules, i.e. a function from altitude to
        /// dimensionless values between 0 (null density) and 1 (maximum density).
        /// Layers must be sorted from bottom to top. The width of the last layer is
        /// ignored, i.e. it always extend to the top atmosphere boundary. At most 2
        /// layers can be specified.
        DensityProfileLayer* _rayleighDensity;

        /// The scattering coefficient of air molecules at the altitude where their
        /// density is maximum (usually the bottom of the atmosphere), as a function
        /// of wavelength, in m^-1. The scattering coefficient at altitude h is equal
        /// to 'rayleigh_scattering' times 'rayleigh_density' at this altitude. This
        /// vector must have the same size as the wavelengths parameter.
        std::vector<double> _rayleighScattering;

        /// The density profile of aerosols, i.e. a function from altitude to
        /// dimensionless values between 0 (null density) and 1 (maximum density).
        /// Layers must be sorted from bottom to top. The width of the last layer is
        /// ignored, i.e. it always extend to the top atmosphere boundary. At most 2
        /// layers can be specified.
        DensityProfileLayer* _mieDensity;

        /// The scattering coefficient of aerosols at the altitude where their
        /// density is maximum (usually the bottom of the atmosphere), as a function
        /// of wavelength, in m^-1. The scattering coefficient at altitude h is equal
        /// to 'mie_scattering' times 'mie_density' at this altitude. This vector
        /// must have the same size as the wavelengths parameter.
        /// </summary>
        std::vector<double> _mieScattering;

        /// The extinction coefficient of aerosols at the altitude where their
        /// density is maximum (usually the bottom of the atmosphere), as a function
        /// of wavelength, in m^-1. The extinction coefficient at altitude h is equal
        /// to 'mie_extinction' times 'mie_density' at this altitude. This vector
        /// must have the same size as the wavelengths parameter.
        std::vector<double> _mieExtinction;

        /// The asymetry parameter for the Cornette-Shanks phase function for the aerosols.
        double _miePhaseFunctionG;

        /// The density profile of air molecules that absorb light (e.g. ozone), i.e.
        /// a function from altitude to dimensionless values between 0 (null density)
        /// and 1 (maximum density). Layers must be sorted from bottom to top. The
        /// width of the last layer is ignored, i.e. it always extend to the top
        /// atmosphere boundary. At most 2 layers can be specified.
        std::vector<DensityProfileLayer*> _absorptionDensity;

        /// The extinction coefficient of molecules that absorb light (e.g. ozone) at
        /// the altitude where their density is maximum, as a function of wavelength,
        /// in m^-1. The extinction coefficient at altitude h is equal to
        /// 'absorption_extinction' times 'absorption_density' at this altitude. This
        /// vector must have the same size as the wavelengths parameter.
        std::vector<double> _absorptionExtinction;

        /// The average albedo of the ground, as a function of wavelength. This
        /// vector must have the same size as the wavelengths parameter.
        std::vector<double> _groundAlbedo;

        /// The maximum Sun zenith angle for which atmospheric scattering must be
        /// precomputed, in radians (for maximum precision, use the smallest Sun
        /// zenith angle yielding negligible sky light radiance values. For instance,
        /// for the Earth case, 102 degrees is a good choice for most cases (120
        /// degrees is necessary for very high exposure values).
        double _maxSunZenithAngle;

        /// Use radiance or luminance mode.
        Luminance _luminance;

        /// The number of wavelengths for which atmospheric scattering must be
        /// precomputed (the temporary GPU memory used during precomputations, and
        /// the GPU memory used by the precomputed results, is independent of this
        /// number, but the precomputation time is directly proportional to this number):
        /// - if this number is less than or equal to 3, scattering is precomputed
        /// for 3 wavelengths, and stored as irradiance values. Then both the
        /// radiance-based and the luminance-based API functions are provided (see
        /// the above note).
        /// - otherwise, scattering is precomputed for this number of wavelengths
        /// (rounded up to a multiple of 3), integrated with the CIE color matching
        /// functions, and stored as illuminance values. Then only the
        /// luminance-based API functions are provided (see the above note).
        [[nodiscard]] inline int num_precomputed_wavelengths() const {
            return _luminance == Luminance::Precomputed ? 15 : 3;
        }

        /// Whether to use half precision floats (16 bits) or single precision floats
        /// (32 bits) for the precomputed textures. Half precision is sufficient for
        /// most cases, except for very high exposure values.
        bool _halfPrecision;

        TextureBuffer* _textureBuffer;
    };
} // namespace Sky::Atm
