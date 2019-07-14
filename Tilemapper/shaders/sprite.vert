#version 330 core

layout (location = 0) in vec2 quad_vert;

// Instanced data
layout (location = 1) in ivec4 vert_src_bounds;
layout (location = 4) in uint flags; // includes cset
layout (location = 2) in vec2 position;
layout (location = 3) in int layer;

uniform mat4 camera;

out vec2 frag_uv;
flat out ivec4 frag_src_bounds;
flat out uint frag_cset;
flat out vec4 color_filter;
flat out uint show_color0;

/*
flags packing:
HVDR RRRR - GGGG GBBB - BBAA AAAS - CCCC CCCC
R: (1 - red) component for color filter
G: (1 - green) component for color filter
B: (1 - blue) component for color filter
C: colorset index (0-255)
H: Horizontal flip
V: Vertical flip
D: Diagonal flip (transposes x and y)
S: 1 to show color0
*/

const uint HFLIP       = 0x80000000u;
const uint VFLIP       = 0x40000000u;
const uint DFLIP       = 0x20000000u;
const uint SHOW_COLOR0 = 0x00000100u;

void main() {
    vec2 size = vec2(vert_src_bounds.zw);
    bool dflip = (DFLIP & flags) != 0u;

    gl_Position = camera * vec4(
        position + quad_vert * (dflip? size.yx : size.xy),
        float(layer),
        1.0
    );

    frag_src_bounds = vert_src_bounds;
    frag_cset = flags & 0xFFu;
    color_filter = vec4(
        1.0 - float((flags & 0x1F000000u) >> 24u) / 31.0,
        1.0 - float((flags & 0x00F80000u) >> 19u) / 31.0,
        1.0 - float((flags & 0x0007C000u) >> 14u) / 31.0,
        1.0 - float((flags & 0x00003E00u) >>  9u) / 31.0
    );

    vec2 uv = dflip? quad_vert.yx : quad_vert.xy;
    frag_uv = vec2(
        bool(HFLIP & flags)? 1.0 - uv.x : uv.x,
        bool(VFLIP & flags)? 1.0 - uv.y : uv.y
    );
    show_color0 = flags & SHOW_COLOR0;
}
