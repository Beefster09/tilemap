#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "text.h"

constexpr int ASCII_START = 33;

int Font::print(GlyphRenderData* const buffer, size_t buf_size, const char* const text, float x, float y) {
	int len = 0;
	int x_offset = 0;
	int y_offset = 0;
	u32 rgba = 0xFFFFFFFF;
	for (const char* c = text; *c; c++) {
		if (*c == '\n') {
			x_offset = 0;
			y_offset += line_height;
		}
		else if (*c == ' ') {
			x_offset += space_width;
		}
		else if (*c == '#') { // Color control character; double up to get literal #
			c++;
			if (*c == '#') goto handle_ascii; // I'm sorry for being a terrible person :(
			auto end = strchr(c, ';');
			if (!end) {
				fprintf(stderr, "TEXT ERROR: Set Color Control Code is not followed with ';'.\n  From: \"%s\"\n", text);
				return -1;
			}
			auto count = (u32)(end - c);
			for (int i = 0; i < count; i++) {
				if (!(c[i] >= '0' && c[i] <= '9'
				  || c[i] >= 'A' && c[i] <= 'F'
				  || c[i] >= 'a' && c[i] <= 'f')) {
					fprintf(stderr, "TEXT ERROR: non-hex-digit character found in Set Color Control Code.\n  From: \"%s\"\n", text);
					return -1;
				}
			}
			switch (count) {
			case 0: {
				rgba = 0xFFFFFFFF;
				break;
			}
			case 3: {  // RGB
				break;
			}
			case 4: {  // RGBA
				break;
			}
			case 6: {  // RRGGBB
				rgba = strtoul(c, nullptr, 16);
				rgba <<= 8;
				rgba |= 0xff;
				break;
			}
			case 8: {  // RRGGBBAA
				rgba = strtoul(c, nullptr, 16);
				break;
			}
			default: {
				fprintf(stderr, "TEXT ERROR: Set Color Control Code has an unsupported number of hex digits.\n  From: \"%s\"\n", text);
				return -1;
			}
			}
			c = end;
		}
		else if (*c >= ASCII_START && *c < 127) {
			handle_ascii:

			if (len >= buf_size) {
				fprintf(stderr, "WARNING: Unable to render the text \"%s\" with only %d glyphs.\n", text, buf_size);
				return len;
			}

			auto& glyph = ascii_glyphs[*c - ASCII_START];
			buffer[len++] = {
				x + x_offset + glyph.offset_x, y + y_offset + glyph.offset_y,
				glyph.src_x, glyph.src_y, glyph.src_w, glyph.src_h,
				rgba
			};
			x_offset += glyph.advance + kern_pairs[c[0]][c[1]];
		}
		// TODO: UTF-8
	}
	return len;
}

#define X 255,
#define _ 0,
// The bitmap needs padding Because openGL apparently has a minimum texture size.
#define PADDING 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
const u8 SIMPLE_FONT_BITMAP[] = {
	//  0           1           2           3           4               5           6           7           8           9
	_ X X X _   _ X X _ _   _ X X X _   _ X X X _   X _ _ _ X       X X X X X   _ X X X _   X X X X X   _ X X X _   _ X X X _      PADDING
	X _ _ _ X   _ _ X _ _   X _ _ _ X   X _ _ _ X   X _ _ _ X       X _ _ _ _   X _ _ _ X   _ _ _ _ X   X _ _ _ X   X _ _ _ X      PADDING
	X _ _ _ X   _ _ X _ _   _ _ _ _ X   _ _ _ _ X   X _ _ _ X       X _ _ _ _   X _ _ _ _   _ _ _ X _   X _ _ _ X   X _ _ _ X      PADDING
	X _ X _ X   _ _ X _ _   _ _ X X _   _ _ X X _   X X X X X       X X X X _   X X X X _   _ _ _ X _   _ X X X _   _ X X X X      PADDING
	X _ _ _ X   _ _ X _ _   _ X _ _ _   _ _ _ _ X   _ _ _ _ X       _ _ _ _ X   X _ _ _ X   _ _ X _ _   X _ _ _ X   _ _ _ _ X      PADDING
	X _ _ _ X   _ _ X _ _   X _ _ _ _   X _ _ _ X   _ _ _ _ X       X _ _ _ X   X _ _ _ X   _ _ X _ _   X _ _ _ X   X _ _ _ X      PADDING
	_ X X X _   X X X X X   X X X X X   _ X X X _   _ _ _ _ X       _ X X X _   _ X X X _   _ _ X _ _   _ X X X _   _ X X X _      PADDING
	//  A           B           C           D           E               F           G           H           I           J
	_ _ X _ _   X X X X _   _ X X X _   X X X X _   X X X X X       X X X X X   _ X X X _   X _ _ _ X   X X X X X   _ X X X X      PADDING
	_ X _ X _   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ _       X _ _ _ _   X _ _ _ X   X _ _ _ X   _ _ X _ _   _ _ _ X _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ _   X _ _ _ X   X _ _ _ _       X _ _ _ _   X _ _ _ _   X _ _ _ X   _ _ X _ _   _ _ _ X _      PADDING
	X _ _ _ X   X X X X _   X _ _ _ _   X _ _ _ X   X X X X _       X X X X _   X _ X X X   X X X X X   _ _ X _ _   _ _ _ X _      PADDING
	X X X X X   X _ _ _ X   X _ _ _ _   X _ _ _ X   X _ _ _ _       X _ _ _ _   X _ _ _ X   X _ _ _ X   _ _ X _ _   _ _ _ X _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ _       X _ _ _ _   X _ _ _ X   X _ _ _ X   _ _ X _ _   X _ _ X _      PADDING
	X _ _ _ X   X X X X _   _ X X X _   X X X X _   X X X X X       X _ _ _ _   _ X X X _   X _ _ _ X   X X X X X   _ X X _ _      PADDING
	//  K           L           M           N           O               P           Q           R           S           T
	X _ _ _ X   X _ _ _ _   X _ _ _ X   X _ _ _ X   _ X X X _       X X X X _   _ X X X _   X X X X _   _ X X X _   X X X X X      PADDING
	X _ _ X _   X _ _ _ _   X X _ X X   X X _ _ X   X _ _ _ X       X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	X _ X _ _   X _ _ _ _   X _ X _ X   X _ X _ X   X _ _ _ X       X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ _   _ _ X _ _      PADDING
	X X _ _ _   X _ _ _ _   X _ _ _ X   X _ X _ X   X _ _ _ X       X X X X _   X _ _ _ X   X X X X _   _ X X X _   _ _ X _ _      PADDING
	X _ X _ _   X _ _ _ _   X _ _ _ X   X _ X _ X   X _ _ _ X       X _ _ _ _   X _ X _ X   X _ _ X _   _ _ _ _ X   _ _ X _ _      PADDING
	X _ _ X _   X _ _ _ _   X _ _ _ X   X _ _ X X   X _ _ _ X       X _ _ _ _   X _ _ X _   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	X _ _ _ X   X X X X X   X _ _ _ X   X _ _ _ X   _ X X X _       X _ _ _ _   _ X X _ X   X _ _ _ X   _ X X X _   _ _ X _ _      PADDING
	//  U           V           W           X           Y               Z      !,.;:
	X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X       X X X X X   X X _ _ _   X X X X _   _ X X X _   X X X X X      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X       _ _ _ _ X   X X _ _ _   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   _ X _ X _   _ X _ X _       _ _ _ X _   X X _ _ X   X _ _ _ X   X _ _ _ _   _ _ X _ _      PADDING
	X _ _ _ X   _ X _ X _   X _ _ _ X   _ _ X _ _   _ _ X _ _       _ _ X _ _   X X _ _ X   X X X X _   _ X X X _   _ _ X _ _      PADDING
	X _ _ _ X   _ X _ X _   X _ X _ X   _ X _ X _   _ _ X _ _       _ X _ _ _   _ _ _ _ X   X _ _ X _   _ _ _ _ X   _ _ X _ _      PADDING
	X _ _ _ X   _ _ X _ _   X X _ X X   X _ _ _ X   _ _ X _ _       X _ _ _ _   X X _ X _   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	_ X X X _   _ _ X _ _   X _ _ _ X   X _ _ _ X   _ _ X _ _       X X X X X   X X _ _ X   X _ _ _ X   _ X X X _   _ _ X _ _      PADDING
	//  a           b           c           d           e               f           g           h       i    j
	_ _ _ _ _   X _ _ _ _   _ _ _ _ _   _ _ _ _ X   _ _ _ _ _       _ _ X X _   _ X X X _   X _ _ _ _   X _ _ _ X   _ _ _ _ _      PADDING
	_ _ _ _ _   X _ _ _ _   _ _ _ _ _   _ _ _ _ X   _ _ _ _ _       _ X _ _ _   X _ _ _ X   X _ _ _ _   _ _ _ _ _   _ _ _ _ _      PADDING
	_ X X X _   X X X X _   _ X X X _   _ X X X X   _ X X X _       _ X _ _ _   X _ _ _ X   X X X X _   X _ _ _ X   _ _ _ _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X       X X X _ _   X _ _ _ X   X _ _ _ X   X _ _ _ X   _ _ _ _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ _   X _ _ _ X   X X X X _       _ X _ _ _   _ X X X X   X _ _ _ X   X _ _ _ X   _ _ _ _ _      PADDING
	X _ _ X X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ _       _ X _ _ _   _ _ _ _ X   X _ _ _ X   X _ _ _ X   _ _ _ _ _      PADDING
	_ X X _ X   X X X X _   _ X X X _   _ X X X X   _ X X X X       _ X _ _ _   _ X X X _   X _ _ _ X   X _ _ _ X   _ _ _ _ _      PADDING
	//  k           l           m           n           o               p           q           r           s           t
	X _ _ _ _   X _ _ _ _   _ _ _ _ _   _ _ _ _ _   _ _ _ _ _       X X X X _   _ X X X _   _ _ _ _ _   _ X _ _ X   _ X _ _ _      PADDING
	X _ _ _ _   X _ _ _ _   _ _ _ _ _   _ _ _ _ _   _ _ _ _ _       X _ _ _ X   X _ _ _ X   _ _ _ _ _   _ _ X X _   _ X _ _ _      PADDING
	X _ _ X _   X _ _ _ _   X X _ X _   X X X X _   _ X X X _       X _ _ _ X   X _ _ _ X   _ X X X _   _ X X X X   X X X _ _      PADDING
	X _ X _ _   X _ _ _ _   X _ X _ X   X _ _ _ X   X _ _ _ X       X _ _ _ X   X _ _ _ X   X _ _ _ _   X _ _ _ _   _ X _ _ _      PADDING
	X X _ _ _   X _ _ _ _   X _ X _ X   X _ _ _ X   X _ _ _ X       X X X X _   _ X X X X   X _ _ _ _   _ X X X _   _ X _ _ _      PADDING
	X _ X _ _   X _ _ _ _   X _ X _ X   X _ _ _ X   X _ _ _ X       X _ _ _ _   _ _ _ _ X   X _ _ _ _   _ _ _ _ X   _ X _ _ _      PADDING
	X _ _ X _   X _ _ _ _   X _ X _ X   X _ _ _ X   _ X X X _       X _ _ _ _   _ _ _ _ X   X _ _ _ _   X X X X _   _ _ X _ _      PADDING
	//  u           v           w           x           y               z
	_ _ _ _ _   _ _ _ _ _   _ _ _ _ _   _ _ _ _ _   X _ _ _ X       _ _ _ _ _   _ X X X _   X X X X _   _ X X X _   X X X X X      PADDING
	_ _ _ _ _   _ _ _ _ _   _ _ _ _ _   _ _ _ _ _   X _ _ _ X       _ _ _ _ _   X _ _ _ X   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X   X _ _ _ X       X X X X X   X _ _ _ X   X _ _ _ X   X _ _ _ _   _ _ X _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ _ _ X   _ X _ X _   X _ _ _ X       _ _ _ X _   X _ _ _ X   X X X X _   _ X X X _   _ _ X _ _      PADDING
	X _ _ _ X   X _ _ _ X   X _ X _ X   _ _ X _ _   _ X X X X       _ _ X _ _   X _ X _ X   X _ _ X _   _ _ _ _ X   _ _ X _ _      PADDING
	X _ _ _ X   _ X _ X _   X X _ X X   _ X _ X _   _ _ _ _ X       _ X _ _ _   X _ _ X _   X _ _ _ X   X _ _ _ X   _ _ X _ _      PADDING
	_ X X X _   _ _ X _ _   X _ _ _ X   X _ _ _ X   _ X X X _       X X X X X   _ X X _ X   X _ _ _ X   _ X X X _   _ _ X _ _      PADDING
};
#undef X
const i32 LOWERCASE_WIDTHS[] = {
	//  a  b  c  d  e    f  g  h  i  j
		5, 5, 5, 5, 5,   4, 5, 5, 1, 4,
	//  k  l  m  n  o    p  q  r  s  t
		4, 1, 5, 5, 5,   5, 5, 4, 5, 3,
	//  u  v  w  x  y    z
		5, 5, 5, 5, 5,   5
};
//const char LOWERCASE_TAILS[] = "gpqy";

constexpr int SIMPLE_FONT_BITMAP_WIDTH = 64;
constexpr int SIMPLE_FONT_BITMAP_HEIGHT = 7 * 7;

Font simple_font;

void init_simple_font() {
	GLuint tex_handle;
	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_RECTANGLE, tex_handle);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, SIMPLE_FONT_BITMAP_WIDTH, SIMPLE_FONT_BITMAP_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, SIMPLE_FONT_BITMAP);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	simple_font.glyph_atlas = new Texture(tex_handle, GL_TEXTURE_RECTANGLE);

	for (int i = 0; i <= 9; i++) {
		simple_font.ascii_glyphs['0' + i - ASCII_START] = { 5 * i, 0, 5, 7, 6 };
	}
	for (int i = 0; i <= 'Z' - 'A'; i++) {
		simple_font.ascii_glyphs['A' + i - ASCII_START] = { 5 * (i % 10), 7 * (1 + i / 10), 5, 7, 6 };
	}
	for (int i = 0; i <= 'z' - 'a'; i++) {
		if (i == 'j' - 'a' || i == 's' - 'a') continue; // j and s are odd exceptions
		simple_font.ascii_glyphs['a' + i - ASCII_START] = { 5 * (i % 10), 7 * (4 + i / 10), LOWERCASE_WIDTHS[i], 7, LOWERCASE_WIDTHS[i] + 1 };
	}
	simple_font.ascii_glyphs['j' - ASCII_START] = { 41, 28, 4, 9, 5 };
	simple_font.ascii_glyphs['s' - ASCII_START] = { 40, 37, 5, 5, 6, 0, 2 };
	for (const char* tail = "gpqy"; *tail; tail++) {
		simple_font.ascii_glyphs[*tail - ASCII_START].offset_y = 2;
	}
#define GLYPH(CH, ...) simple_font.ascii_glyphs[CH - ASCII_START] = { __VA_ARGS__ }
	GLYPH('!', 30, 21, 2, 7, 3);
	GLYPH(':', 30, 23, 2, 5, 3, 0, 1);
	GLYPH(';', 30, 23, 2, 6, 3, 0, 1);
	GLYPH('.', 30, 26, 2, 2, 3, 0, 5);
	GLYPH(',', 30, 26, 2, 3, 3, 0, 5);

	simple_font.kern_pairs['L']['Y'] = -1;
	simple_font.kern_pairs['L']['V'] = -1;
	for (const char* c = "acdegmnopqrsuvwxyz"; *c; c++) {
		simple_font.kern_pairs['f'][*c] = -1;
	}
	simple_font.line_height = 10;
	simple_font.space_width = 4;
}
