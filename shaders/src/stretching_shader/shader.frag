#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform StretchingDescriptor {
    vec3 local_min;
    vec3 local_max;
    vec3 global_max;
};
void main() {
    vec4 texel = texture(u_tex, in_texcoord);

    texel.r = ((texel.r - local_min.r) / (local_max.r - local_min.r)) * (global_max.r);
    texel.g = ((texel.g - local_min.g) / (local_max.g - local_min.g)) * (global_max.g);
    texel.b = ((texel.b - local_min.b) / (local_max.b - local_min.b)) * (global_max.b);

    fragment = texel;
}