#pragma once

#include <unordered_map>
#include "common.h"

struct Font;

struct GlyphRenderData {
	float x, y;
	i32 src_x, src_y, src_w, src_h;
	u32 rgba;
};

struct FontDims {
	i32 space_width;
	i32 line_height;
};

int print_glyphs(const Font* font, GlyphRenderData* const buffer, size_t buf_size, const char* text, float x, float y);
FontDims get_font_dimensions(const Font& font);
int bind_font_glyph_atlas(Font& font, int slot = 0);

void init_simple_font();

constexpr char TEXT_CURSOR      = '\01';
constexpr char TEXT_CURSOR_END  = '\02';
constexpr int TEXT_CURSOR_RADIX = 10;