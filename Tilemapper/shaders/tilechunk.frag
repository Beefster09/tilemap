#version 330 core

in vec2 frag_uv;
flat in uint frag_tile;
flat in uint frag_cset;
flat in vec4 color_filter;

uniform usampler2DArray tileset;
uniform samplerBuffer palette;
uniform float alpha = 1.0;
uniform bool transparent_color0;

out vec4 frag_color;

void main() {
    uint color_index = texture(tileset, vec3(frag_uv, float(frag_tile))).r % 4u;
    if (color_index == 0u && transparent_color0) discard;
    frag_color = vec4(texelFetch(palette, int(frag_cset * 4u + color_index)).rgb, clamp(alpha, 0.0, 1.0)) * color_filter;
}
