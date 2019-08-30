#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "text.h"

// TODO: portability
#include <intrin.h>

constexpr int ASCII_START = 33;
constexpr int ASCII_END = 127; // exclusive range
constexpr int ASCII_SIZE = ASCII_END - ASCII_START;
constexpr int TAB_LENGTH = 8; // Number of spaces that make up a tab
constexpr int CURSOR_GLYPH_ID = 94;

// @console
HexColor cursor_color = 0xccccccff;

constexpr u64 LARGE_PRIME = 541894198537;
constexpr u64 LEFT_PRIME  = 926153;
constexpr u64 RIGHT_PRIME = 1698461;
constexpr u64 INVERSE_PHI64 = 11400714819323198485;
constexpr u64 ALLBITS64 = 0xFFFFffffFFFFffff;

static u64 hash_kern_pair(int left, int right) {
	u64 pair = (((u64)left * LEFT_PRIME) << 32ull) + ((u64)right * RIGHT_PRIME);
	// Xorshift64
	pair ^= pair << 13;
	pair ^= pair >> 7;
	pair ^= pair << 17;
	// The high bits are better on xorshift, so we'll reorder those to the front
	return _rotl64(pair, 32);
}

//constexpr int UNICODE_SIZE = 1 << 20;
void kern_hash_func_analysis() {
	u64 bits_set_total = 0;
	for (int left = 0; left < 65536; left++) {
		for (int right = 0; right < 65536; right++) {
			auto hash = hash_kern_pair(left, right);
			bits_set_total += __popcnt64(hash);
		}
		if (left % 2048 == 0) {
			printf("Completed %d codepoints (left side).\n", left);
		}
	}
	double avg = ((double)bits_set_total) / (65536.0 * 65536.0);
	printf("Average number of bits set for hashes in Unicode BMP: %f\n", avg);
}

struct KernPair {
	char32_t left, right;
	i32 kern_offset;
};

struct KernTableEntry {
	char32_t left, right;
	u32 probe_count;
	i32 kern_offset;
};

struct KerningData {
	KernTableEntry* table;
	u32 capacity;
	u32 shift;
	u32 max_probe_count;
};

struct GlyphData {
	i32 src_x, src_y, src_w, src_h;
	i32 advance;
	i32 offset_x = 0, offset_y = 0;
};

struct Font {
	Texture* glyph_atlas;
	Texture* glyph_table;
	GLuint glyph_table_buffer;
	GlyphData ascii_glyphs[ASCII_SIZE]; // glyphs from 0x21-0x7E
	struct {
		i32 src_x, src_y, src_w, src_h;
		i32 offset_x = 0, offset_y = 0;
	} cursor;
	i32 space_width;
	i32 line_height;
	KerningData kerning;
	// std::unordered_map<u32, GlyphData> other_glyphs;
};

constexpr int PROBE_MULT = 3;
constexpr int PROBE_SKIP = 7;

static void kern_table_insert(KerningData& kerning, KernPair pair) {
	auto hash = hash_kern_pair(pair.left, pair.right);
	auto index = (hash * INVERSE_PHI64) >> kerning.shift;
	auto mask = ALLBITS64 >> kerning.shift;
	DBG_LOG("Kerning Hash %lc %lc ==> %016llx (index %lld)", pair.left, pair.right, hash, index);
	u16 probe_count = 1;
	bool stole;
	do {
		stole = false;
		u16 richest = UINT16_MAX;
		u64 richest_index = index;
		u16 richest_my_probe_count = probe_count;
		while (kerning.table[index].probe_count > 0) {
			auto& cur = kerning.table[index];
			if (cur.left == pair.left && cur.right == pair.right) {
				cur.kern_offset = pair.kern_offset;
				return;
			}
			DBG_LOG(
				"Hash collision: %lc %lc vs %lc %lc on probe #%hd (@%d, vs. #%hd)",
				pair.left, pair.right,
				cur.left, cur.right,
				probe_count,
				index,
				cur.probe_count
			);

			if (cur.probe_count < richest) { // remember the richest guy to (maybe) steal from
				richest = cur.probe_count;
				richest_index = index;
				richest_my_probe_count = probe_count;
			}

			if (probe_count > richest) {
				DBG_LOG(
					"Stealing index %d from %lc %lc for %lc %lc",
					richest_index,
					kerning.table[richest_index].left, kerning.table[richest_index].right,
					pair.left, pair.right
				);
				// steal from the rich
				KernTableEntry entry = {
					(u32) pair.left, (u32) pair.right,
					richest_my_probe_count,
					pair.kern_offset,
				};
				std::swap(entry, kerning.table[richest_index]);
				// continue on with the victim
				pair = {
					entry.left, entry.right,
					entry.kern_offset
				};
				probe_count = entry.probe_count;
				index = richest_index;
				stole = true;
			}

			index += PROBE_MULT * probe_count + PROBE_SKIP;
			index &= mask;
			probe_count += 1;

			if (probe_count > kerning.max_probe_count) {
				kerning.max_probe_count = probe_count;
			}
			if (stole) break;
		}
	} while (stole);
	kerning.table[index] = {
		(u32) pair.left, (u32) pair.right,
		probe_count,
		pair.kern_offset,
	};
}

static KerningData create_kern_table(const KernPair* const kern_pairs, const unsigned int n_kern_pairs) {
	// Get next largest power of 2 beyond a 80% load factor
	u32 capacity = 1 << (32 - __lzcnt(n_kern_pairs * 10 / 8)); // TODO: make this more portable
	u32 mask = capacity - 1;
	auto table_data = (KernTableEntry*)calloc(capacity, sizeof(KernTableEntry));
	KerningData out = { table_data, capacity, __lzcnt64((u64) mask), 0 };
	for (int i = 0; i < n_kern_pairs; i++) {
		kern_table_insert(out, kern_pairs[i]);
	}
	DBG_LOG("Kerning table max probe = %d", out.max_probe_count);
#ifndef NDEBUG
	printf("Kerning table probe count distribution:\n");
	for (int base = 0; base < capacity; base += 16) {
		printf("   ");
		for (int i = base; i < base + 16; i++) {
			printf(" %d", out.table[i].probe_count);
		}
		printf("\n");
	}
	printf("\n");
#endif
	return out;
}

static int get_kerning_offset(const KerningData& const kerning, char32_t left, char32_t right) {
	if (left <= ' ' || right <= ' ') return 0;
	auto index = (hash_kern_pair(left, right) * INVERSE_PHI64) >> kerning.shift;
	auto mask = ALLBITS64 >> kerning.shift;
	for (int probe_count = 1; probe_count <= kerning.max_probe_count; probe_count++) {
		auto& entry = kerning.table[index];
		if (entry.probe_count == 0) return 0;
		if (entry.left == left && entry.right == right) return entry.kern_offset;
		index += PROBE_MULT * probe_count + PROBE_SKIP;
		index &= mask;
	}
	return 0;
}

static inline bool handle_color_code(const char* const fulltext, const char* const c, HexColor* color, const char** end) {
	*end = strchr(c, ']');
	if (!*end) {
		ERR_LOG("TEXT ERROR: Set Color Control Code is not followed with ']'.\n  From: \"%s\"", fulltext);
		return false;
	}
	switch (parse_hex_color(c, nullptr, color)) {
	case OK: return true;
	case INVALID_CHARS:
		ERR_LOG("TEXT ERROR: non-hex-digit character found in Set Color Control Code.\n  From: \"%s\"", fulltext);
		return false;
	case INVALID_LEN:
		ERR_LOG("TEXT ERROR: Set Color Control Code has an unsupported number of hex digits.\n  From: \"%s\"", fulltext);
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
				ERR_LOG("Found '#' at end of string '%s'", text);
				goto handle_ascii;
			default:
				ERR_LOG("Unsupported control code #%c in string '%s'", c[1], text);
				return -1;
			}
		}
		else if (*c >= ASCII_START && *c < ASCII_END) {
			handle_ascii:

			if (len >= buf_size) {
				ERR_LOG("WARNING: Unable to render the text \"%s\" with only %zd glyphs.", text, buf_size);
				return len;
			}

			u32 glyph_id = *c - ASCII_START;
			auto& glyph = font->ascii_glyphs[glyph_id];
			buffer[len++] = {
				x + x_offset + glyph.offset_x,
				y + y_offset + glyph.offset_y,
				glyph_id,
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
			x + cursor_x + font->cursor.offset_x,
			y + cursor_y + font->cursor.offset_y,
			CURSOR_GLYPH_ID,
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

int bind_font_glyph_table(Font& font, int slot) {
	return font.glyph_table->bind(slot);
}

#include "generated/simple_font.h"

void init_simple_font() {
	GLuint tex_handles[2];
	glGenTextures(2, tex_handles);
	auto& atlas = tex_handles[0];
	auto& table = tex_handles[1];
	glBindTexture(GL_TEXTURE_RECTANGLE, atlas);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, simple_font__bitmap_width, simple_font__bitmap_height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, simple_font__bitmap);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	u32 glyph_bounds[(ASCII_SIZE + 1) * 4];
	for (int i = 0; i < ASCII_SIZE; i++) {
#define SET_GLYPH_ATTR(NAME, OFFSET) glyph_bounds[4 * i + OFFSET] = simple_font.ascii_glyphs[i].src_ ## NAME;
		SET_GLYPH_ATTR(x, 0);
		SET_GLYPH_ATTR(y, 1);
		SET_GLYPH_ATTR(w, 2);
		SET_GLYPH_ATTR(h, 3);
#undef SET_GLYPH_ATTR
	}
	glyph_bounds[4 * CURSOR_GLYPH_ID + 0] = simple_font.cursor.src_x;
	glyph_bounds[4 * CURSOR_GLYPH_ID + 1] = simple_font.cursor.src_y;
	glyph_bounds[4 * CURSOR_GLYPH_ID + 2] = simple_font.cursor.src_w;
	glyph_bounds[4 * CURSOR_GLYPH_ID + 3] = simple_font.cursor.src_h;

	GLuint table_buffer;
	glGenBuffers(1, &table_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, table_buffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glyph_bounds), (void*)glyph_bounds, GL_STATIC_DRAW);

	glBindTexture(GL_TEXTURE_BUFFER, table);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, table_buffer);

	simple_font.glyph_atlas = new Texture(atlas, GL_TEXTURE_RECTANGLE);
	simple_font.glyph_table = new Texture(table, GL_TEXTURE_BUFFER);
	simple_font.glyph_table_buffer = table_buffer;
	simple_font.kerning = create_kern_table(simple_font__kerning, simple_font__n_kern_pairs);

	//kern_hash_func_analysis();
}
