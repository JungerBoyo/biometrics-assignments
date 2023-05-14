#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(std140, binding = 5) uniform Transform {
    float scale;
    float offset_x;
    float offset_y;
    float aspect_ratio;
    uint flip_tex_y_axis_xor;
};

layout(location = 0) out vec2 out_texcoord;

void main() { 
    out_texcoord = vec2(in_texcoord.x, float((uint(in_texcoord.y) ^ flip_tex_y_axis_xor) & 0x1));

    gl_Position = vec4(in_position.x, in_position.y, 0.0, 1.0);
}