#version 450 core

layout(location = 0) out vec4 in_fragment;

layout(std140, binding = 5) uniform Transform {
    float scale;
    float offset_x;
    float offset_y;
    float aspect_ratio;
    uint flip_tex_y_axis_xor;
    vec4 drawing_cursor_color;
};

void main() { 
    in_fragment = drawing_cursor_color;
}
