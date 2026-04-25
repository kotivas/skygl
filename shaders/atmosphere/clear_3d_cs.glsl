#version 450
#include <constants.glsl>

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = LOCAL_SIZE) in;
layout (binding = 0, rgba32f) uniform image3D targetImage;

void main() {
    ivec3 uv = ivec3(gl_GlobalInvocationID);
    imageStore(targetImage, uv, vec4(0.0, 0.0, 0.0, 0.0));
}