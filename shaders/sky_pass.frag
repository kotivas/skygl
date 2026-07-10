#version 450 core
#include <atmosphere/atmosphere.glsl>

out vec4 FragColor;
in vec3 vRay;

uniform vec2 uSunSize;
uniform vec3 uCameraPos;
uniform float uTime;
uniform vec3 uSunDir;

void main() {
    vec3 transmittance;
    vec3 earth_center = vec3(0, -bottom_radius, 0);
    vec3 camera = uCameraPos - earth_center;
    vec3 sun_dir = normalize(uSunDir);
    vec3 view_ray = normalize(vRay);

    // ==== SKY ====
    vec3 sky = GetSkyRadiance(camera, view_ray, 0.0, sun_dir, transmittance);
    
    FragColor = vec4(sky, 1.0);
}