#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "text.h"

constexpr int ASCII_START = 33;
constexpr int TAB_LENGTH = 8; // Number of spaces that make up a tab

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
	switch (count) {
	case 0: {
		*color = 0xFFFFFFFF;
		break;
	}
	case 3: {  // RGB
		break;
	}
	case 4: {  // RGBA
		break;
	}
	case 6: {  // RRGGBB
		*color = strtoul(c, nullptr, 16);
		*color <<= 8;
		*color |= 0xff;
		break;
	}
	case 8: {  // RRGGBBAA
		*color = strtoul(c, nullptr, 16);
		break;
	}
	default: {
		fprintf(stderr, "TEXT ERROR: Set Color Control Code has an unsupported number of hex digits.\n  From: \"%s\"\n", fulltext);
		return false;
	}
	}
	return true;
}

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
		else if (*c == '\t') {
			auto tab = TAB_LENGTH * space_width;
			x_offset = x_offset / tab * tab + tab;
		}
		else if (*c == ' ') {
			x_offset += space_width;
		}
		else if (*c == '#') { // Control character; double up to get literal #
			switch (c[1]) {
			case '#':
				c++; goto handle_ascii;
			case 'c': case 'C': { // color
				if (c[2] != '[') {
					fprintf(stderr, "Found color control without '[' in string '%s'", text);
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
			case 0: {
				fprintf(stderr, "Found '#' at end of string '%s'\n", text);
				goto handle_ascii;
			}
			default: {
				fprintf(stderr, "Unsupported control code #%c in string '%s'\n", c[1], text);
				return -1;
			}
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
			x_offset += glyph.advance + kern_pairs[c[0]][c[1]];
		}
		// TODO: UTF-8
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

	for (int i = 0; i < sizeof(simple_font__kerning) / sizeof(int); i += 3) {
		simple_font.kern_pairs[simple_font__kerning[i]][simple_font__kerning[i + 1]] = simple_font__kerning[i + 2];
	}
}
