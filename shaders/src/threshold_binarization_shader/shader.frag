#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;


layout(std140, binding = 6) uniform BinarizationDescriptor {
    float threshold;
    // 0 = BINARIZE_CHANNEL_ALL
    // 1 = BINARIZE_CHANNEL_R
    // 2 = BINARIZE_CHANNEL_G
    // 3 = BINARIZE_CHANNEL_B
    int channel;
};
void main() {
    vec4 texel = texture(u_tex, in_texcoord);
    switch (channel) {
    case 0: 
        float mean = (texel.r + texel.g + texel.b) / 3.0;
        texel.r = texel.g = texel.b = mean > threshold ? 1.0 : 0.0;
        break;
    case 1:
        texel.r = texel.r > threshold ? 1.0 : 0.0;
        break;
    case 2:
        texel.g = texel.g > threshold ? 1.0 : 0.0;
        break;            
    case 3:
        texel.b = texel.b > threshold ? 1.0 : 0.0;
        break;            
    }

    fragment = texel;
}