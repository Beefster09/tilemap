#pragma once

#include <unordered_map>
#include "common.h"

struct Texture;

struct GlyphData {
	i32 src_x, src_y, src_w, src_h;
	i32 advance;
	i32 offset_x = 0, offset_y = 0;
	// TODO? kerning pairs
};

struct GlyphRenderData {
	float x, y;
	i32 src_x, src_y, src_w, src_h;
	u32 rgba;
};

struct Font {
	Texture* glyph_atlas;
	GlyphData ascii_glyphs[94]; // glyphs from 0x21-0x7E
	i32 space_width;
	i32 line_height;
	// std::unordered_map<u32, GlyphData> other_glyphs;
	std::unordered_map<u32, std::unordered_map<u32, i32>> kern_pairs; // TODO probably: replace with something that doesn't add elements on a failed lookup.

	int print(GlyphRenderData* const buffer, size_t buf_size, const char* const text, float x, float y);
};

void init_simple_font();