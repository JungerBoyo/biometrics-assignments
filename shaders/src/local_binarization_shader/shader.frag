#version 450 core

layout(location = 0) out vec4 fragment;

layout(binding = 5) uniform sampler2D u_tex;

layout(location = 0) in vec2 in_texcoord;

// mean * (1 + pow * exp(-q * mean)) + ratio * (std_dev/div - 1)

// niblack - mean + ratio * stddev 
// savoula - mean + mean * ratio * stddev/(div-1)
// phansca - mean + mean * pow * exp(-q*mean) + mean * ratio * stddev/(div-1)

layout(std140, binding = 6) uniform LocalBinarizationDescriptor {
    int kernel_size;
    int equation_type;
    float ratio;
    float standard_deviation_div;
    float pow;
    float q;
};
void main() {
    ivec2 tex_size = textureSize(u_tex, 0);
    ivec2 tex_index = ivec2(in_texcoord * vec2(tex_size));

    int kernel_area_size = (2 * kernel_size + 1) * (2 * kernel_size + 1);

    float sum = 0.0;
    float sum_of_squares = 0.0;
    for (int v=-kernel_size; v<=kernel_size; ++v) {
        for (int u=-kernel_size; u<=kernel_size; ++u) {
            vec4 texel = texelFetch(u_tex, tex_index + ivec2(u, v), 0);
            float value = (texel.r + texel.g + texel.b) / 3.0;
            sum += value;
            sum_of_squares += value * value;
        }
    }

    float mean = sum / kernel_area_size;
    float variance = (sum_of_squares - (sum * sum)/kernel_area_size)/kernel_area_size;
    float standard_deviation = sqrt(max(variance, 0.0));

    float threshold = 0.0;
    switch (equation_type) 
    {
        // niblack
        case 0: 
            threshold = mean + ratio * standard_deviation; 
            break;
        // savoula 
        case 1: 
            threshold = mean + mean * ratio * standard_deviation/(standard_deviation_div - 1); 
            break;
        // phanscalar
        case 2: 
            threshold = mean + mean * pow * exp(-q*mean) + mean * ratio * standard_deviation/(standard_deviation_div - 1); 
            break; 
    }

    vec4 texel = texture(u_tex, in_texcoord);
    float texel_mean = (texel.r + texel.g + texel.b) / 3.0;
    texel.r = texel.g = texel.b = texel_mean > threshold ? 1.0 : 0.0;

    fragment = texel;
}