#version 330 core

uniform vec4 color = vec4(0.0, 0.0, 0.0, 0.5);

out vec4 frag_color;

void main() {
    frag_color = color;
}
