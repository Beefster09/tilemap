#version 330 core

layout (location = 0) in vec2 vert_pos;
layout (location = 1) in uint vert_tile;
layout (location = 2) in uint vert_cset;

uniform vec2 offset;
uniform mat4 camera;
uniform mat4 transform;
uniform int chunk_size;

const int tile_size = 16;

out vec2 frag_pos;
flat out uint frag_tile;
flat out uint frag_cset;
out vec2 frag_uv;

const uint HFLIP = 0x80000000u;
const uint VFLIP = 0x40000000u;
const uint DFLIP = 0x20000000u;

void main() {
    vec2 world_pos = vec2(
        (vert_pos.x + float(gl_InstanceID % chunk_size)) * float(tile_size) + offset.x,
        (vert_pos.y + float(gl_InstanceID / chunk_size)) * float(tile_size) + offset.y
    );
    if (vert_tile == 0u) { // tile 0 is reserved as no-display
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
    else {
        gl_Position = camera * vec4(world_pos, 0.0, 1.0);
    }
    frag_pos = world_pos;
    frag_tile = (vert_tile - 1u) & 0x00FFFFFFu;
    frag_cset = vert_cset;
    vec2 uv = bool(DFLIP & vert_tile)? vert_pos.yx : vert_pos.xy;
    frag_uv = vec2( // top 2 bits can be used to flip
        bool(HFLIP & vert_tile)? 1.0 - uv.x : uv.x,
        bool(VFLIP & vert_tile)? 1.0 - uv.y : uv.y
    );
}
