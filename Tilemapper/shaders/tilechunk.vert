#version 330 core

layout (location = 0) in vec2 vert_pos;
layout (location = 1) in uint vert_tile;
layout (location = 2) in uint vert_cset;

uniform vec2 offset;
uniform mat4 camera;
uniform mat4 transform;
uniform int chunk_size = 4;
uniform int tile_size = 16;
uniform uint flags;

out vec2 frag_pos;
out vec2 frag_uv;
flat out uint frag_tile;
flat out uint frag_cset;
flat out uint frag_flags;
flat out float frag_alpha;
flat out vec3 color_filter;

const uint HFLIP = 0x80000000u;
const uint VFLIP = 0x40000000u;
const uint DFLIP = 0x20000000u;
const uint TILE_MASK = 0x0001FFFFu;

/*
cset packing:
RRRR RGGG - GGBB BBB? - DPAA AAAA - CCCC CCCC
R: (1 - red) component for color filter
G: (1 - green) component for color filter
B: (1 - blue) component for color filter
D: Dither toggle
P: Dither parity toggle
A: (1 - alpha)
C: colorset (0-255)
*/

/*
tile packing:
HVD? ???? - ???? ???T - TTTT TTTT - TTTT TTTT
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
    frag_tile = (vert_tile - 1u) & TILE_MASK;
    frag_cset = vert_cset & 0xFFu;
    frag_alpha = 1.0 - (float((vert_cset & 0x3F00u) >> 8u) / 63.0);
    frag_flags = flags ^ ((vert_cset & 0x0001C000u) >> 13u);
    color_filter = vec3(
        1.0 - float((vert_cset & 0xF8000000u) >> 27u) / 31.0,
        1.0 - float((vert_cset & 0x07C00000u) >> 22u) / 31.0,
        1.0 - float((vert_cset & 0x003E0000u) >> 17u) / 31.0
    );
    vec2 uv = bool(DFLIP & vert_tile)? vert_pos.yx : vert_pos.xy;
    frag_uv = vec2( // top 2 bits can be used to flip
        bool(HFLIP & vert_tile)? 1.0 - uv.x : uv.x,
        bool(VFLIP & vert_tile)? 1.0 - uv.y : uv.y
    );
}
