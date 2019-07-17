#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "text.h"

constexpr int ASCII_START = 33;
constexpr int TAB_LENGTH = 8; // Number of spaces that make up a tab

static u32 cursor_color = 0xeeeeeeff;

static inline u32 double_nybbles(u32 x) {
	// expand ABCD into _A_B_C_D, shift & combine
	u32 r =  x & 0x000F;
	r    |= (x & 0x00F0) << 4;
	r    |= (x & 0x0F00) << 8;
	r    |= (x & 0xF000) << 12;
	return r | (r << 4);
}

static bool handle_color_code(const char* const fulltext, const char* const c, u32* color, const char** end) {
	*end = strchr(c, ']');
	if (!*end) {
		fprintf(stderr, "TEXT ERROR: Set Color Control Code is not followed with ']'.\n  From: \"%s\"\n", fulltext);
		return false;
	}
	auto count = (u32)(*end - c);
	for (int i = 0; i < count; i++) {
		if (!(c[i] >= '0' && c[i] <= '9'
			|| c[i] >= 'A' && c[i] <= 'F'
			|| c[i] >= 'a' && c[i] <= 'f')) {
			fprintf(stderr, "TEXT ERROR: non-hex-digit character found in Set Color Control Code.\n  From: \"%s\"\n", fulltext);
			return false;
		}
	}
	if (count == 0) {
		*color = 0xFFFFFFFF;
		return true;
	}
	*color = strtoul(c, nullptr, 16);
	switch (count) {
	case 3:  // RGB
		*color = double_nybbles(*color);
	// drop-thru intentional
	case 6:  // RRGGBB
		*color <<= 8;
		*color |= 0xff;
		return true;
	
	case 4:  // RGBA
		*color = double_nybbles(*color);
	// drop-thru intentional
	case 8:  // RRGGBBAA
		return true;
	
	default:
		fprintf(stderr, "TEXT ERROR: Set Color Control Code has an unsupported number of hex digits.\n  From: \"%s\"\n", fulltext);
		return false;
	}
}

static int get_kerning_offset(const KernPair* const kerning, const int n_kern_pairs, int left, int right) {
	if (left <= ' ' || right <= ' ') return 0;
	int lo = 0, hi = n_kern_pairs, mid = n_kern_pairs / 2;
	while (lo < hi) {
		int cmp = kerning[mid].left - left;
		if (cmp == 0) {
			cmp = kerning[mid].right - right;
			if (cmp == 0) return kerning[mid].kern_offset;
		}

		if (cmp < 0) {
			lo = mid + 1;
		}
		else {  //if (cmp < 0)
			hi = mid;
		}
		mid = lo + ((hi - lo) / 2);
	}
	return 0;
}

int Font::print(GlyphRenderData* const buffer, size_t buf_size, const char* text, float x, float y) const {
	int len = 0;
	int x_offset = 0;
	int y_offset = 0;
	u32 rgba = 0xFFFFFFFF;
	// cursor data
	const char* cursor_char = nullptr;
	bool cursor_set = false;
	int cursor_x = 0;
	int cursor_y = 0;
	if (text[0] == TEXT_CURSOR) {
		char* end;
		auto cursor_offset = strtoul(text + 1, &end, TEXT_CURSOR_RADIX);
		text = end;
		cursor_char = text + cursor_offset + 1;
	}
	for (const char* c = text; *c; c++) {
		if (c == cursor_char) {
			cursor_x = x_offset;
			cursor_y = y_offset;
			cursor_set = true;
		}
		if (*c == '\n') {
			x_offset = 0;
			y_offset += line_height;
		}
		else if (*c == '\t') {
			auto tab = TAB_LENGTH * space_width;
			x_offset = x_offset / tab * tab + tab;
		}
		else if (*c == '\r') {
			x_offset = 0;
		}
		else if (*c == ' ') {
			x_offset += space_width;
		}
		else if (*c == '#') { // Control character; double up to get literal #
			switch (c[1]) { // Note: goto used to interpret a botched control sequence literally
			case '#':
				c++; // Interpret the next '#' literally
				goto handle_ascii;
			case 'c': case 'C': { // color
				if (c[2] != '[') {
					fprintf(stderr, "Found color control without '[' in string '%s'", text);
					goto handle_ascii;
				}
				const char* end;
				if (handle_color_code(text, c + 3, &rgba, &end)) {
					c = end;
				}
				else {
					return -1;
				}
				break;
			}
			case '0': { // Reset all attributes
				c++;
				rgba = 0xFFFFFFFF;
				break;
			}
			case 0:
				fprintf(stderr, "Found '#' at end of string '%s'\n", text);
				goto handle_ascii;
			default:
				fprintf(stderr, "Unsupported control code #%c in string '%s'\n", c[1], text);
				return -1;
			}
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
			x_offset += glyph.advance + get_kerning_offset(kern_pairs, n_kern_pairs, c[0], c[1]);
		}
		else if (*c > 127) {

		}
		// TODO: UTF-8
	}
	if (cursor_char != nullptr && len < buf_size) {
		if (!cursor_set) {
			cursor_x = x_offset;
			cursor_y = y_offset;
		}
		buffer[len++] = {
			x + cursor_x + cursor.offset_x, y + cursor_y + cursor.offset_y,
			cursor.src_x, cursor.src_y, cursor.src_w, cursor.src_h,
			cursor_color
		};
	}
	return len;
}

#include "generated/simple_font.h"

void init_simple_font() {
	GLuint tex_handle;
	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_RECTANGLE, tex_handle);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, simple_font__bitmap_width, simple_font__bitmap_height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, simple_font__bitmap);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	simple_font.glyph_atlas = new Texture(tex_handle, GL_TEXTURE_RECTANGLE);
}
