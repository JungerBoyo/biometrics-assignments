#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform EqualizationDescriptor {
    int range;
    float distributant_r0;
    float distributant_g0;
    float distributant_b0;
    vec4 distributant_r[64];
    vec4 distributant_g[64];
    vec4 distributant_b[64];
};

float distributant_r_at(uint i) {
    return distributant_r[i >> 2][i & 0x3];
}
float distributant_g_at(uint i) {
    return distributant_g[i >> 2][i & 0x3];
}
float distributant_b_at(uint i) {
    return distributant_b[i >> 2][i & 0x3];
}

void main() {
    vec4 texel = texture(u_tex, in_texcoord);

    float k = float(range - 1) / 255.0;

    texel.r = ((distributant_r_at(uint(texel.r * 255.0)) - distributant_r0) / (1.0 - distributant_r0)) * k;
    texel.g = ((distributant_g_at(uint(texel.g * 255.0)) - distributant_g0) / (1.0 - distributant_g0)) * k;
    texel.b = ((distributant_b_at(uint(texel.b * 255.0)) - distributant_b0) / (1.0 - distributant_b0)) * k;

    fragment = texel;
}