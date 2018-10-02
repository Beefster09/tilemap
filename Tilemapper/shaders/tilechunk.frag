#version 330 core

in vec2 frag_pos;
in vec2 frag_uv;
flat in uint frag_tile;
flat in uint frag_cset;
flat in uint frag_flags;
flat in float frag_alpha;
flat in vec3 color_filter;

uniform usampler2DArray tileset;
uniform sampler2DRect palette;

out vec4 frag_color;

const uint SHOW_COLOR0   = 0x1u;
const uint DITHER        = 0x4u;
const uint DITHER_PARITY = 0x2u;
// 0x8u is togglable with global flags but is currently unused

void main() {
    // vec2 tile_pixel = frag_uv * textureSize(tileset, 0).xy;
    uint color_index = texture(tileset, vec3(frag_uv, float(frag_tile))).r % uint(textureSize(palette, 0).x);
    if (
        (color_index == 0u && (frag_flags & SHOW_COLOR0) == 0u)
        || ( // Dither filtering
            (frag_flags & DITHER) != 0u
            && (uint(frag_pos.x) + uint(frag_pos.y)) % 2u == ((frag_flags & DITHER_PARITY) >> 1)
        )
    ) discard;
    frag_color = vec4(texelFetch(palette, ivec2(color_index, frag_cset)).rgb * color_filter, frag_alpha);
}
