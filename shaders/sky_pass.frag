#version 450 core
#include <atmosphere/atmosphere.glsl>

out vec4 FragColor;
in vec3 vRay;

uniform vec2 uSunSize;
uniform vec3 uCameraPos;
uniform float uTime;
uniform vec3 uSunDir;

//vec3 Moon(vec3 dir) {
//    vec3 moonDir = normalize(-uSunDir);
//    float mu = dot(dir, moonDir);
//
//    vec3 up      = abs(moonDir.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0);
//    vec3 right   = normalize(cross(up, moonDir));
//    vec3 localUp = normalize(cross(moonDir, right));
//
//    float u = dot(dir, right);
//    float v = dot(dir, localUp);
//
//    vec2 moonSize = vec2(tan(0.027), cos(0.027));
//
//    float scale = 0.8 / moonSize.x;
//    vec2 uv = vec2(u, v) * scale * 0.5 + 0.5;
//
//    float sunDisk = smoothstep(moonSize.y - 0.00005, moonSize.y, mu);
//    float halo    = exp(-max(0.0, acos(mu) - moonSize.x));
//    float halosmall    = exp(-max(0.0, acos(mu) - moonSize.x) * 50.0);
//
//    float moon = texture(moonTexture, uv).r;
//
//    return vec3(5000) * (moon * sunDisk + halo * 0.0005 + halosmall * 0.01);
//}

void main() {
    vec3 transmittance;
    vec3 earth_center = vec3(0, -bottom_radius, 0);
    vec3 camera = uCameraPos - earth_center;
    vec3 sun_dir = normalize(uSunDir);
    vec3 view_ray = normalize(vRay);
    float mu = dot(view_ray, sun_dir);

    // ==== SKY & SUN ====
    vec3 sky = GetSkyRadiance(camera, view_ray, 0.0, sun_dir, transmittance);

    float sunDisk = smoothstep(uSunSize.y, uSunSize.y, mu);
    float halo    = exp(-max(0.0, acos(mu) - uSunSize.x) * 2000);
    vec3 sky_luminance = GetSolarRadiance() * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    sky += sky_luminance * (sunDisk + halo) * transmittance;

    //
    //    float r = length(camera);
    //    float mu = dot(camera, normalize(vRay)) / r;
    //    if (RayIntersectsGround(r, mu)){
    //        sky = vec3(10);//10000
    //        float fog = exp(-length(vRay - uCameraPos) / 1000);
    //        sky *= fog;
    //    }

    FragColor = vec4(sky, 1.0);
}