#pragma once

#include <unordered_map>
#include "common.h"

class Texture;

struct GlyphData {
	i32 src_x, src_y, src_w, src_h;
	i32 advance;
	i32 offset_x = 0, offset_y = 0;
};

struct GlyphRenderData {
	float x, y;
	i32 src_x, src_y, src_w, src_h;
	u32 rgba;
};

struct KernPair {
	int left, right;
	int kern_offset;
};

struct Font {
	Texture* glyph_atlas;
	const GlyphData ascii_glyphs[94]; // glyphs from 0x21-0x7E
	struct {
		i32 src_x, src_y, src_w, src_h;
		i32 offset_x = 0, offset_y = 0;
	} cursor;
	const i32 space_width;
	const i32 line_height;
	// std::unordered_map<u32, GlyphData> other_glyphs;
	// TODO: replace with something that doesn't add elements on a failed lookup.
	const KernPair* kern_pairs;
	const size_t n_kern_pairs;

	int print(GlyphRenderData* const buffer, size_t buf_size, const char* text, float x, float y) const;
};

void init_simple_font();

constexpr char TEXT_CURSOR     = '\01';
constexpr char TEXT_CURSOR_END = '\02';
constexpr int TEXT_CURSOR_RADIX = 10;