#version 450 core

layout(location = 0) in vec2 in_position;

layout(std140, binding = 5) uniform Transform {
    float scale;
    float offset_x;
    float offset_y;
    float aspect_ratio;
    uint flip_tex_y_axis_xor;
    vec4 drawing_cursor_color;
};

void main() { 
    gl_Position = vec4(scale*in_position.x + offset_x, scale*aspect_ratio*in_position.y + offset_y, -1.0, 1.0);
}