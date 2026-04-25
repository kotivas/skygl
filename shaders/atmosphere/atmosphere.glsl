// common atmosphere functions, uniforms and constants
#include <uniforms.glsl>
#include <utility.glsl>
#include <transmittance_functions.glsl>
#include <scattering_functions.glsl>
#include <irradiance_functions.glsl>
#include <rendering_functions.glsl>

// ===== Bruneton textures =====
uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler2D irradiance_texture;
uniform sampler3D single_mie_scattering_texture;

// ===== CONSTANTS ====
const vec3 kSphereCenter = vec3(0.0, 1.0, 0.0);
const float kSphereRadius = 1.0;
const vec3 kSphereAlbedo = vec3(0.8, 0.8, 0.8);
const vec3 kGroundAlbedo = vec3(0.04, 0.04, 0.04);

// ============================================================
// ------------------------ BRUNETON --------------------------
// ============================================================

Luminance3 GetSolarRadiance() {
    return solar_irradiance / (PI * sun_angular_radius * sun_angular_radius) * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
}
// ------------------------------------------------------------------
Luminance3 GetSkyRadiance(Position camera, Direction view_ray, Length shadow_length,
Direction sun_dir, out DimensionlessSpectrum transmittance) {
    return GetSkyRadiance(transmittance_texture, scattering_texture, single_mie_scattering_texture,
    camera, view_ray, shadow_length, sun_dir, transmittance) *
    SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
}
// ------------------------------------------------------------------
Luminance3 GetSkyRadianceToPoint(Position camera, Position _point, Length shadow_length,
Direction sun_dir, out DimensionlessSpectrum transmittance) {
    return GetSkyRadianceToPoint(transmittance_texture, scattering_texture, single_mie_scattering_texture,
    camera, _point, shadow_length, sun_dir, transmittance) *
    SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
}
// ------------------------------------------------------------------
Illuminance3 GetSunAndSkyIrradiance(Position p, Direction normal, Direction sun_dir, out IrradianceSpectrum sky_irradiance) {
    IrradianceSpectrum sun_irradiance = GetSunAndSkyIrradiance(
    transmittance_texture, irradiance_texture, p, normal,
    sun_dir, sky_irradiance);
    sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    return sun_irradiance * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
}
