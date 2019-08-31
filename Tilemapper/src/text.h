#pragma once

#include "common.h"

struct Font;

struct GlyphRenderData {
	float x, y;
	u32 glyph_id;
	u32 rgba;
};

struct FontDims {
	i32 space_width;
	i32 line_height;
};

int print_glyphs(const Font* font, GlyphRenderData* const buffer, size_t buf_size, const char* text, float x, float y);
FontDims get_font_dimensions(const Font& font);
int bind_font_glyph_atlas(Font& font, int slot = 0);
int bind_font_glyph_table(Font& font, int slot = 0);

void init_simple_font();

constexpr char TEXT_CURSOR      = '\01';
constexpr char TEXT_CURSOR_END  = '\02';
constexpr int TEXT_CURSOR_RADIX = 10;