#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D skyTexture;
uniform sampler2D cloudsTexture;

uniform float uExposure;
uniform float uGamma;

const float WHITE_POINT = 11.07f;

vec3 HableTonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
vec3 Hable(vec3 color, float exp, float whitePoint) {
    return HableTonemap(color * exp) / HableTonemap(vec3(whitePoint));
}

void main() {
    vec3 sky = texture(skyTexture, TexCoords).rgb;
    vec4 clouds = texture(cloudsTexture, TexCoords);

    //    vec4 color = vec4(mix(sky, clouds.rgb, clouds.a), 1.0);
    vec4 color = vec4(sky * (1.0 - clouds.a) + clouds.rgb, 1.0);

    color.rgb = Hable(color.rgb, uExposure, WHITE_POINT);
    color.rgb = pow(color.rgb, vec3(1.0 / uGamma));

    FragColor = color;
}