#version 330 core

in vec2 frag_uv;
flat in ivec4 frag_src_bounds;
flat in uint frag_cset;
flat in vec4 color_filter;
flat in uint show_color0;

uniform usampler2DRect spritesheet;
uniform samplerBuffer palette;

out vec4 frag_color;

void main() {
    vec2 size = vec2(frag_src_bounds.zw);
    uint color_index = texelFetch(spritesheet, frag_src_bounds.xy + ivec2(size * frag_uv)).r % 4u;
    if ((color_index | show_color0) == 0u) discard;
    frag_color = vec4(texelFetch(palette, int(frag_cset * 4u + color_index)).rgb, 1.0) * color_filter;
}
