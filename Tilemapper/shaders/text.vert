#version 330 core

layout (location = 0) in vec2 quad_vert;

// Instanced data
layout (location = 1) in ivec4 vert_src_bounds;
layout (location = 2) in vec2 position;
layout (location = 3) in uint rgba;

uniform mat4 camera;
uniform float layer = 50.0;

out vec2 frag_uv;
flat out ivec4 frag_src_bounds;
flat out vec4 color_filter;

void main() {
    vec2 size = vec2(vert_src_bounds.zw);
    gl_Position = camera * vec4(
        position + quad_vert * size,
        layer,
        1.0
    );

    frag_src_bounds = vert_src_bounds;
    frag_uv = quad_vert;
    color_filter = vec4(
        float((rgba & 0xFF000000u) >> 24u) / 255.0,
        float((rgba & 0x00FF0000u) >> 16u) / 255.0,
        float((rgba & 0x0000FF00u) >>  8u) / 255.0,
        float( rgba & 0x000000FFu        ) / 255.0
    );
}
