#version 330 core

in vec2 frag_uv;

uniform sampler2D virtual_screen;

out vec4 frag_color;

float center (float pix_coord) {
    // return pix_coord;
    float norm = (fract(pix_coord) - 0.5) * 2.0;
    return floor(pix_coord) + clamp(pow(norm, 3.0) / 2.0 + 0.5, 0.0, 1.0);
}

void main() {
    frag_color = texture(virtual_screen, frag_uv);
    // vec2 vres = textureSize(virtual_screen, 0);
    // frag_color = texture(virtual_screen, vec2(
    //     center(frag_uv.x * vres.x) / vres.x,
    //     center(frag_uv.y * vres.y) / vres.y
    // ));
    // frag_color = vec4(
    //     center(frag_uv.x * vres.x) / vres.x,
    //     center(frag_uv.y * vres.y) / vres.y,
    //     0.0, 1.0
    // );
}
