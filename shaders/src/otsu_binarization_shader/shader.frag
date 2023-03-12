#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform BinarizationDescriptor {
    float threshold;
};
void main() {
    vec4 texel = texture(u_tex, in_texcoord);

    float mean = (texel.r + texel.g + texel.b) / 3.0;
    texel.r = texel.g = texel.b = mean > threshold ? 1.0 : 0.0;

    fragment = texel;
}