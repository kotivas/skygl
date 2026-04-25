#ifndef UTILITY_GLSL
#define UTILITY_GLSL

float remap(float value, float min1, float max1, float min2, float max2) {
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

vec4 triplanar(sampler2D tex, vec3 p, vec3 n, float scale) {
    vec3 blend = abs(n);
    blend = pow(blend, vec3(4.0));
    blend /= dot(blend, vec3(1.0));

    vec4 cx = texture(tex, p.yz * scale);
    vec4 cy = texture(tex, p.xz * scale);
    vec4 cz = texture(tex, p.xy * scale);

    return cx * blend.x + cy * blend.y + cz * blend.z;
}

float raySphereIntersectFar(vec3 r0, vec3 rd, vec3 s0, float sr) {
    float a = dot(rd, rd);
    vec3 s0_r0 = r0 - s0;
    float b = 2.0 * dot(rd, s0_r0);
    float c = dot(s0_r0, s0_r0) - (sr * sr);

    float d = b*b - 4.0*a*c;
    if (d < 0.0) return -1.0;

    return (-b + sqrt(d)) / (2.0 * a);
}

float raySphereIntersect(vec3 r0, vec3 rd, vec3 s0, float sr) {
    float a = dot(rd, rd);
    vec3 s0_r0 = r0 - s0;
    float b = 2.0 * dot(rd, s0_r0);
    float c = dot(s0_r0, s0_r0) - (sr * sr);

    float d = b*b - 4.0*a*c;
    if (d < 0.0) return -1.0;

    float sqrtD = sqrt(d);

    float t0 = (-b - sqrtD) / (2.0 * a);
    if (t0 > 0.0) return t0;

    float t1 = (-b + sqrtD) / (2.0 * a);
    if (t1 > 0.0) return t1;

    return -1.0;
}

#endif