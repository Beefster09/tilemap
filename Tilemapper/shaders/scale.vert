#version 330 core

layout (location = 0) in vec2 vert_pos;

out vec2 frag_uv;

void main() {
    gl_Position = vec4(vert_pos * 2.0 - 1.0, 0.0, 1.0);
    frag_uv = vec2(vert_pos.x, 1.0 - vert_pos.y);
}
