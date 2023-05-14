#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform GlobalFillDescriptor {
    float r_min;
    float r_max;
    float g_min;
    float g_max;
    float b_min;
    float b_max;
    vec3 color;
};

void main() {
    vec4 texel = texture(u_tex, in_texcoord);

    if (texel.r >= r_min && texel.r <= r_max &&
        texel.g >= g_min && texel.g <= g_max &&
        texel.b >= b_min && texel.b <= b_max) {
        fragment = vec4(color, texel.a);            
    } else {
        fragment = texel;
    }
}