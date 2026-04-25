#version 450
#include <constants.glsl>
#include <uniforms.glsl>
#include <utility.glsl>
#include <transmittance_functions.glsl>

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
layout (binding = 0, rgba32f) uniform image2D transmittance;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 frag_coord = coord + vec2(0.5, 0.5);
    imageStore(transmittance, coord, vec4(ComputeTransmittanceToTopAtmosphereBoundaryTexture(frag_coord), 1.0));
}