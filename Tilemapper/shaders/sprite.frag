#version 330 core

in vec2 frag_uv;
flat in ivec4 frag_src_bounds;
flat in uint frag_cset;
flat in vec4 color_filter;
flat in uint show_color0;

uniform usampler2DRect spritesheet;
uniform sampler2DRect palette;

out vec4 frag_color;

void main() {
    vec2 size = vec2(frag_src_bounds.zw);
    uint color_index = texelFetch(spritesheet, frag_src_bounds.xy + ivec2(size * frag_uv)).r % uint(textureSize(palette, 0).x);
    if ((color_index | show_color0) == 0u) discard;
    frag_color = vec4(texelFetch(palette, ivec2(color_index, frag_cset)).rgb, 1.0) * color_filter;
    // float u = frag_uv.x;
    // float v = frag_uv.y - 0.2;
    // frag_color = vec4(
    //     vec2(frag_src_bounds.xy + ivec2(size * frag_uv)) * 0.01,
    //     u * u + v * v < 0.2,
    //     1.0
    // ) * color_filter;
}
