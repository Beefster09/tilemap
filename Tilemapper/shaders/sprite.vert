#version 330 core

layout (location = 0) in vec2 quad_vert;

// Instanced data
layout (location = 1) in ivec4 vert_src_bounds;
layout (location = 2) in uint cset;
layout (location = 3) in vec2 position;
layout (location = 4) in vec2 pivot;
layout (location = 5) in float rotation;
layout (location = 6) in float layer;

uniform mat4 camera;

out vec2 frag_uv;
flat out ivec4 frag_src_bounds;
flat out uint frag_cset;
flat out vec4 color_filter;

void main() {
    vec2 rel_to_pivot = quad_vert - pivot;
    float c = cos(rotation) * float(vert_src_bounds.z - vert_src_bounds.x);
    float s = sin(rotation) * float(vert_src_bounds.w - vert_src_bounds.y);
    mat2 transform = mat2(c, -s, s, c);
    gl_Position = camera * vec4(transform * rel_to_pivot + pivot, layer, 0.0);

    frag_src_bounds = vert_src_bounds;
    frag_cset = cset & 0xFFu;
    color_filter = vec4(
        1.0 - float((cset & 0xFC000000u) >> 26u) / 63.0,
        1.0 - float((cset & 0x03F00000u) >> 20u) / 63.0,
        1.0 - float((cset & 0x000FC000u) >> 14u) / 63.0,
        1.0 - float((cset & 0x00003F00u) >> 8u) / 63.0
    );
    frag_uv = quad_vert;
}
