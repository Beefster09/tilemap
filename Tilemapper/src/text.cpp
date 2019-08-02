#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "text.h"

// TODO: portability
#include <intrin.h>

constexpr int ASCII_START = 33;
constexpr int TAB_LENGTH = 8; // Number of spaces that make up a tab

// @console
HexColor cursor_color = 0xccccccff;

constexpr u64 LARGE_PRIME = 541894198537;
constexpr int LEFT_PRIME  = 926153;
constexpr int RIGHT_PRIME = 1698461;
static u64 hash_kern_pair(int left, int right) {
	u64 pair = ((left * LEFT_PRIME) << 20) + (right * RIGHT_PRIME);
	// Xorshift64
	pair ^= pair << 13;
	pair ^= pair >> 7;
	pair ^= pair << 17;
	// The high bits are better, but let's multiply by a large prime for good measure
	return (pair >> 32) * LARGE_PRIME;
}

struct KernPair {
	int left, right;
	int kern_offset;
};

struct KernTableEntry {
	u32 left, right;
	i32 kern_offset;
	u16 probe_count;
	bool occupied;
	u8 _padding_;
};

struct KerningData {
	KernTableEntry* table;
	u32 capacity;
	u32 mask;
	u32 max_probe_count;
};

struct GlyphData {
	i32 src_x, src_y, src_w, src_h;
	i32 advance;
	i32 offset_x = 0, offset_y = 0;
};

struct Font {
	Texture* glyph_atlas;
	GlyphData ascii_glyphs[94]; // glyphs from 0x21-0x7E
	struct {
		i32 src_x, src_y, src_w, src_h;
		i32 offset_x = 0, offset_y = 0;
	} cursor;
	i32 space_width;
	i32 line_height;
	KerningData kerning;
	// std::unordered_map<u32, GlyphData> other_glyphs;
};

static void kern_table_insert(KerningData& kerning, KernPair pair) { // Uses robin hood probing
	auto hash = hash_kern_pair(pair.left, pair.right);
	auto index = hash & kerning.mask;
	DBG_LOG("Kerning Hash ('%c':'%c') ==> %016llx (index %lld)\n", pair.left, pair.right, hash, index);
	u16 probe_count = 0;
	while (kerning.table[index].occupied) {
		probe_count += 1;
		index += probe_count;
		index &= kerning.mask;
	}
	// TODO maybe: Robin Hood hashing
	kerning.table[index] = {
		(u32) pair.left, (u32) pair.right,
		pair.kern_offset,
		probe_count,
		true
	};
	if (probe_count > kerning.max_probe_count) {
		kerning.max_probe_count = probe_count;
	}
}

static KerningData create_kern_table(const KernPair* const kern_pairs, const unsigned int n_kern_pairs) {
	// Get next largest power of 2 beyond a 80% load factor
	u32 capacity = 1 << (32 - __lzcnt(n_kern_pairs * 10 / 8)); // TODO: make this more portable
	u32 mask = capacity - 1;
	auto table_data = (KernTableEntry*)calloc(capacity, sizeof(KernTableEntry));
	KerningData out = { table_data, capacity, mask, 0 };
	for (int i = 0; i < n_kern_pairs; i++) {
		kern_table_insert(out, kern_pairs[i]);
	}
	DBG_LOG("Kerning table max probe = %d\n\n", out.max_probe_count);
	return out;
}

static int get_kerning_offset(const KerningData& const kerning, int left, int right) {
	if (left <= ' ' || right <= ' ') return 0;
	auto hash = hash_kern_pair(left, right);
	auto index = hash & kerning.mask;
	for (int probe_count = 1; probe_count <= kerning.max_probe_count; probe_count++) {
		auto& entry = kerning.table[index];
		if (!entry.occupied) return 0;
		if (entry.left == left && entry.right == right) return entry.kern_offset;
		index += probe_count;
		index &= kerning.mask;
	}
	return 0;
}

static inline bool handle_color_code(const char* const fulltext, const char* const c, HexColor* color, const char** end) {
	*end = strchr(c, ']');
	if (!*end) {
		ERR_LOG("TEXT ERROR: Set Color Control Code is not followed with ']'.\n  From: \"%s\"\n", fulltext);
		return false;
	}
	switch (parse_hex_color(c, nullptr, color)) {
	case OK: return true;
	case INVALID_CHARS:
		ERR_LOG("TEXT ERROR: non-hex-digit character found in Set Color Control Code.\n  From: \"%s\"\n", fulltext);
		return false;
	case INVALID_LEN:
		ERR_LOG("TEXT ERROR: Set Color Control Code has an unsupported number of hex digits.\n  From: \"%s\"\n", fulltext);
		return false;
	default:
		ERR_LOG("Unkown Color Control Parse Error in \"%s\".", fulltext);
		return false;
	}
}

int print_glyphs(const Font* font, GlyphRenderData* const buffer, size_t buf_size, const char* text, float x, float y) {
	int len = 0;
	int x_offset = 0;
	int y_offset = 0;
	u32 rgba = 0xFFFFFFFF;
	// cursor data
	int cursor_char = -1;
	bool cursor_set = false;
	int cursor_x = 0;
	int cursor_y = 0;
	if (text[0] == TEXT_CURSOR) {
		char* end;
		cursor_char = strtoul(text + 1, &end, TEXT_CURSOR_RADIX);
		end = strchr(end, TEXT_CURSOR_END);
		if (end == nullptr) return 0;
		text = end + 1;
	}
	int current_char = 0;
	for (const char* c = text; *c; c++, current_char++) {
		if (current_char == cursor_char) {
			cursor_x = x_offset;
			cursor_y = y_offset;
			cursor_set = true;
		}
		if (*c == '\n') {
			x_offset = 0;
			y_offset += font->line_height;
		}
		else if (*c == '\t') {
			auto tab = TAB_LENGTH * font->space_width;
			x_offset = x_offset / tab * tab + tab;
		}
		else if (*c == '\r') {
			x_offset = 0;
		}
		else if (*c == ' ') {
			x_offset += font->space_width;
		}
		else if (*c == '#') { // Control character; double up to get literal #
			switch (c[1]) { // Note: goto used to interpret a botched control sequence literally
			case '#':
				c++; // Interpret the next '#' literally
				goto handle_ascii;
			case 'c': case 'C': { // color
				if (c[2] != '[') {
					ERR_LOG("Found color control without '[' in string '%s'", text);
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
				ERR_LOG("Found '#' at end of string '%s'\n", text);
				goto handle_ascii;
			default:
				ERR_LOG("Unsupported control code #%c in string '%s'\n", c[1], text);
				return -1;
			}
		}
		else if (*c >= ASCII_START && *c < 127) {
			handle_ascii:

			if (len >= buf_size) {
				ERR_LOG("WARNING: Unable to render the text \"%s\" with only %zd glyphs.\n", text, buf_size);
				return len;
			}

			auto& glyph = font->ascii_glyphs[*c - ASCII_START];
			buffer[len++] = {
				x + x_offset + glyph.offset_x, y + y_offset + glyph.offset_y,
				glyph.src_x, glyph.src_y, glyph.src_w, glyph.src_h,
				rgba
			};
			x_offset += glyph.advance + get_kerning_offset(font->kerning, c[0], c[1]);
		}
		else if (*c > 127) {

		}
		// TODO: UTF-8
	}
	if (cursor_char >= 0 && len < buf_size) {
		if (!cursor_set) {
			cursor_x = x_offset;
			cursor_y = y_offset;
		}
		buffer[len++] = {
			x + cursor_x + font->cursor.offset_x, y + cursor_y + font->cursor.offset_y,
			font->cursor.src_x, font->cursor.src_y, font->cursor.src_w, font->cursor.src_h,
			cursor_color
		};
	}
	return len;
}


FontDims get_font_dimensions(const Font& font) {
	return { font.space_width, font.line_height };
}
int bind_font_glyph_atlas(Font& font, int slot) {
	return font.glyph_atlas->bind(slot);
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
	simple_font.kerning = create_kern_table(simple_font__kerning, simple_font__n_kern_pairs);

	// do a thing
}
