#version 330 core
layout (location = 0) in vec2 aPos;
out vec3 vRay;

uniform mat4 uView;
uniform mat4 uProjection;

void main(){
    gl_Position = vec4(aPos, 0.0, 1.0);// to clip space

    vec4 rayClip = vec4(aPos, -1.0, 1.0);// z=-1 front plane
    vec4 rayEye = inverse(uProjection) * rayClip;
    rayEye.z = -1.0;
    rayEye.w = 0.0;
    vRay = normalize((inverse(uView) * rayEye).xyz);
}