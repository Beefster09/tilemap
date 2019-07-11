#version 330 core

in vec2 frag_pos;
in vec2 frag_uv;
flat in uint frag_tile;
flat in uint frag_cset;
flat in vec4 color_filter;
flat in uvec2 dither_mult;
flat in uint dither_modulus;
flat in uint dither_phase;
flat in uint dither_parity;

uniform usampler2DArray tileset;
uniform sampler2DRect palette;
uniform uint flags;

out vec4 frag_color;

const uint SHOW_COLOR0   = 0x1u;

void main() {
    uint color_index = texture(tileset, vec3(frag_uv, float(frag_tile))).r % uint(textureSize(palette, 0).x);
    uvec2 dither = uvec2(frag_pos) * dither_mult;
    if (
        (color_index == 0u && (flags & SHOW_COLOR0) == 0u)
        || (((dither.x + dither.y) % dither_modulus == dither_phase) == bool(dither_parity))
    ) discard;
    frag_color = vec4(texelFetch(palette, ivec2(color_index, frag_cset)).rgb, 1.0) * color_filter;
}
