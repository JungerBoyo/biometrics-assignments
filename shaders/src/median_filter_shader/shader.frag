#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

layout(std140, binding = 6) uniform MedianFilterDescriptor {
    int kernel_size;
};

void main() {
    ivec2 tex_size = textureSize(u_tex, 0);
    ivec2 tex_index = ivec2(in_texcoord * vec2(tex_size));

    vec3 arr[49];
    int i=0;
    for (int v=-kernel_size; v<=kernel_size; ++v) {
        for (int u=-kernel_size; u<=kernel_size; ++u) {
            vec3 texel = texelFetch(u_tex, tex_index + ivec2(u, v), 0).rgb;

            arr[i] = texel;
            for (int channel=0; channel<3; ++channel) {
                for (int j=i; j>0 && arr[j][channel] < arr[j - 1][channel]; --j) {
                   float tmp = arr[j][channel];
                   arr[j][channel] = arr[j-1][channel];
                   arr[j-1][channel] = tmp;
                }
                
            }
            ++i;
        }
    }

    vec4 texel = texture(u_tex, in_texcoord);

    fragment = vec4(arr[((2*kernel_size+1)*(2*kernel_size+1))/2], texel.a);
}