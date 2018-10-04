#version 330 core

layout (location = 0) in vec2 vert_pos;
layout (location = 1) in uint vert_tile;
layout (location = 2) in uint vert_cset;

uniform vec2 offset;
uniform mat4 camera;
uniform mat4 transform;
uniform int chunk_size = 4;
uniform int tile_size = 16;

out vec2 frag_pos;
out vec2 frag_uv;
flat out uint frag_tile;
flat out uint frag_cset;
flat out vec4 color_filter;
flat out uvec2 dither_mult;
flat out uint dither_modulus;
flat out uint dither_parity;

const uint HFLIP = 0x00080000u;
const uint VFLIP = 0x00040000u;
const uint DFLIP = 0x00020000u;
const uint TILE_MASK = 0x0001FFFFu;

/*
cset packing:
RRRR RRGG - GGGG BBBB - BBAA AAAA - CCCC CCCC
R: (1 - red) component for color filter
G: (1 - green) component for color filter
B: (1 - blue) component for color filter
A: (1 - alpha)
C: colorset index (0-255)
*/

/*
tile packing:
XXXY YYMM - MMMP HVDT - TTTT TTTT - TTTT TTTT
X: Dither X multiplier
Y: Dither Y multiplier
M: Dither modulus
P: Dither parity (== or != 0)
H: Horizontal flip
V: Vertical flip
D: Diagonal flip (transposes x and y)
T: Tile index
*/

void main() {
    vec2 world_pos = vec2(
        (vert_pos.x + float(gl_InstanceID % chunk_size)) * float(tile_size) + offset.x,
        (vert_pos.y + float(gl_InstanceID / chunk_size)) * float(tile_size) + offset.y
    );
    if ((vert_tile & TILE_MASK) == 0u) { // tile 0 is reserved as no-display
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
    else {
        gl_Position = camera * vec4(world_pos, 0.0, 1.0);
    }
    frag_pos = world_pos;
    frag_tile = (vert_tile & TILE_MASK) - 1u;
    frag_cset = vert_cset & 0xFFu;
    color_filter = vec4(
        1.0 - float((vert_cset & 0xFC000000u) >> 26u) / 63.0,
        1.0 - float((vert_cset & 0x03F00000u) >> 20u) / 63.0,
        1.0 - float((vert_cset & 0x000FC000u) >> 14u) / 63.0,
        1.0 - float((vert_cset & 0x00003F00u) >> 8u) / 63.0
    );
    dither_mult = uvec2(
        ((vert_tile & 0xE0000000u) >> 29u),
        ((vert_tile & 0x1C000000u) >> 26u)
    );
    dither_modulus = ((vert_tile & 0x03E00000u) >> 21u) + 1u;
    dither_parity = uint((vert_tile & 0x00100000u) != 0u);
    vec2 uv = bool(DFLIP & vert_tile)? vert_pos.yx : vert_pos.xy;
    frag_uv = vec2( // top 2 bits can be used to flip
        bool(HFLIP & vert_tile)? 1.0 - uv.x : uv.x,
        bool(VFLIP & vert_tile)? 1.0 - uv.y : uv.y
    );
}
