#version 330 core

in vec2 frag_uv;
flat in ivec4 src_bounds;
flat in vec4 color_filter;

uniform usampler2DRect glyph_atlas;

out vec4 frag_color;

void main() {
    vec2 size = vec2(src_bounds.zw);
    uint alpha = texelFetch(glyph_atlas, src_bounds.xy + ivec2(size * frag_uv)).r;
    if (alpha <= 16u) discard;
    frag_color = vec4(1.0, 1.0, 1.0, float(alpha) / 255.0) * color_filter;
}
