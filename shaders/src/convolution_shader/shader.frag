#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform ConvolutionDescriptor {
    int kernel_size;
    int gray_scale;
    int gradient;
    vec4 kernel[111]; // 441 max kernel size = 10
};

float kernel_at(uint i) {
    return kernel[i >> 2][i & 0x3];
}

void main() {
    ivec2 tex_size = textureSize(u_tex, 0);
    ivec2 tex_index = ivec2(in_texcoord * vec2(tex_size));


    vec3 sum = vec3(0.0);
    if (gradient == 0) {
        int i=0;
        for (int v=-kernel_size; v<=kernel_size; ++v) {
            for (int u=-kernel_size; u<=kernel_size; ++u) {
                vec4 texel = kernel_at(i) * texelFetch(u_tex, tex_index + ivec2(u, v), 0);
                sum += texel.rgb;
                ++i;
            }
        }
    } else {
        int i=0;
        int kernel_side = 2 * kernel_size + 1;
        vec3 dX = vec3(0.0);
        vec3 dY = vec3(0.0);
        for (int v=-kernel_size; v<=kernel_size; ++v) {
            for (int u=-kernel_size; u<=kernel_size; ++u) {
                vec3 texel = texelFetch(u_tex, tex_index + ivec2(u, v), 0).rgb;
                dX += kernel_at(i) * texel;
                dY += kernel_at(kernel_side - 1 - (v + kernel_size) + (u + kernel_size) * kernel_side) * texel;
                ++i;
            }
        }
        sum = sqrt((dX * dX) + (dY * dY));
    }

    if (gray_scale == 1) {
        sum.r = sum.g = sum.b = (sum.r + sum.b + sum.g)/3.0;
    } 
    vec4 texel = texture(u_tex, in_texcoord);

    fragment = vec4(sum, texel.a);
}