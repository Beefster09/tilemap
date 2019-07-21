#version 330 core

layout (location = 0) in vec2 vert_pos;

void main() {
    gl_Position = vec4(vert_pos * 2.0 - 1.0, 0.0, 1.0);
}
