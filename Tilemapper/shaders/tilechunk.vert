#version 330 core

layout (location = 0) in vec2 vert_pos;
layout (location = 1) in uint vert_tile;
layout (location = 2) in uint vert_cset;

uniform vec2 offset;
uniform mat4 camera;
uniform mat4 transform;
uniform int chunk_size = 4;
uniform int tile_size = 16;
uniform float layer = 0.0;

out vec2 frag_uv;
flat out uint frag_tile;
flat out uint frag_cset;
flat out vec4 color_filter;

const uint HFLIP = 0x00080000u;
const uint VFLIP = 0x00040000u;
const uint DFLIP = 0x00020000u;
const uint TILE_MASK = 0x0001FFFFu;

/*
cset packing:
RRRR RRRR - GGGG GGGG - BBBB BBBB - CCCC CCCC
R: (1 - red) component for color filter
G: (1 - green) component for color filter
B: (1 - blue) component for color filter
C: colorset index (0-255)
*/

/*
tile packing:
???? ???? - ???? HVDT - TTTT TTTT - TTTT TTTT
H: Horizontal flip
V: Vertical flip
D: Diagonal flip (transposes x and y)
T: Tile index
*/

void main() {
    vec2 norm_pos = vec2(float(gl_InstanceID % chunk_size), float(gl_InstanceID / chunk_size));
    vec2 world_pos = (vert_pos + norm_pos) * float(tile_size) + offset;
    uint tile_index = vert_tile & TILE_MASK;

    if (tile_index == 0u) { // tile 0 is reserved as no-display
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
    else {
        gl_Position = camera * vec4(world_pos, layer, 1.0);
    }

    frag_tile = tile_index - 1u;
    frag_cset = vert_cset & 0xFFu;
    color_filter = vec4(
        1.0 - float((vert_cset & 0xFF000000u) >> 24u) / 255.0,
        1.0 - float((vert_cset & 0x00FF0000u) >> 16u) / 255.0,
        1.0 - float((vert_cset & 0x0000FF00u) >> 8u) / 255.0,
        1.0
    );

    // Three bits are used to flip the tile across three different axes, allowing
    // for every possible orthogonal orientation (rotations are Diagonal + vertical or horizontal)
    vec2 uv = bool(DFLIP & vert_tile)? vert_pos.yx : vert_pos.xy;
    frag_uv = vec2(
        bool(HFLIP & vert_tile)? 1.0 - uv.x : uv.x,
        bool(VFLIP & vert_tile)? 1.0 - uv.y : uv.y
    );
}
