#version 330 core

in vec2 frag_uv;

uniform sampler2D virtual_screen;
uniform float sharpness = 2.0;

out vec4 frag_color;

float center (float pix_coord) {
    // return pix_coord;
    float norm = (fract(pix_coord) - 0.5) * 2.0;
    float norm2 = norm * norm;
    return floor(pix_coord) + norm * pow(norm2, sharpness) / 2.0 + 0.5;
}

void main() {
    vec2 vres = textureSize(virtual_screen, 0);
    frag_color = texture(virtual_screen, vec2(
        center(frag_uv.x * vres.x) / vres.x,
        center(frag_uv.y * vres.y) / vres.y
    ));
}
