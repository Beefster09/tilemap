
#include <algorithm>

#include <glad/glad.h>
#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer.h"
#include "text.h"
#include "console.h"

constexpr int CHUNK_MAX = 64;
constexpr int SPRITE_MAX = 512;
constexpr int GLYPH_MAX = 256;
constexpr int STRING_STORAGE_SIZE = 1024 * 16;
constexpr int PRINT_CMD_WS_MAX = 32;
constexpr int PRINT_CMD_SS_MAX = 96;

constexpr int CONSOLE_LINE_OFFSET_LEFT = 8;
constexpr int CONSOLE_LINE_OFFSET_BOTTOM = 15;
constexpr int CONSOLE_LINE_SCROLLBACK_SPACING = 3;
constexpr int SCROLLBACK_PADDING_TOP = 5;


struct GlyphPrintData {
	Font* font;
	const char* text;
	float x;
	float y;
};

const float tile_vertices[] = {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f,
};

extern int screen_width, screen_height;
extern Font simple_font;

// Global Renderer State

// @console name=sharpness
float scaling_sharpness = 2.f;

#ifdef NO_EMBED_SHADERS

#define COMPILE_SHADER(V, F) compileShaderFromFiles((V), (F))
#define TILECHUNK_VERT_SHADER = "shaders/tilechunk.vert"
#define TILECHUNK_VERT_SHADER = "shaders/tilechunk.frag"
#define SPRITE_VERT_SHADER = "shaders/sprite.vert"
#define SPRITE_VERT_SHADER = "shaders/sprite.frag"
#define SCALE_VERT_SHADER = "shaders/scale.vert"
#define SCALE_VERT_SHADER = "shaders/scale.frag"

#define TILECHUNK_VERT_SHADER__SRC = "tilechunk.vert"
#define TILECHUNK_VERT_SHADER__SRC = "tilechunk.frag"
#define SPRITE_VERT_SHADER__SRC = "sprite.vert"
#define SPRITE_VERT_SHADER__SRC = "spirte.frag"
#define SCALE_VERT_SHADER__SRC = "scale.vert"
#define SCALE_VERT_SHADER__SRC = "scale.frag"

#else

#define COMPILE_SHADER(V, F) compileShader((V), (F))
#include "generated/shaders.h"

#endif

#define __SHADER(S) COMPILE_SHADER(S ## _VERT_SHADER, S ## _FRAG_SHADER), S ## _VERT_SHADER__SRC, S ## _FRAG_SHADER__SRC
#define __SHADER2(V, F) COMPILE_SHADER(V ## _VERT_SHADER, F ## _FRAG_SHADER), V ## _VERT_SHADER__SRC, F ## _FRAG_SHADER__SRC

#define EXPAND(V) V
#define __S_SL EXPAND(__S) ## _slots
#define __S_SH EXPAND(__S) ## _shader
#define __SLOT(VAR) (__S_SL.VAR) = (__S_SH).getSlot(#VAR)
#ifdef TEXT // Sometimes this is defined. I don't need it. Fuck that macro.
#undef TEXT
#endif
Renderer::Renderer(GLFWwindow* window, int width, int height):
	window(window),
	tile_shader(__SHADER(TILECHUNK)),
	sprite_shader(__SHADER(SPRITE)),
	scale_shader(__SHADER(SCALE)),
	text_shader(__SHADER(TEXT)),
	overlay_shader(__SHADER(OVERLAY)),
	chunks(CHUNK_MAX),
	sprites(SPRITE_MAX)
{
#define __S tile
#include "generated/tilechunk_uniforms.h"
#undef __S

#define __S scale
#include "generated/scale_uniforms.h"
#undef __S

#define __S sprite
#include "generated/sprite_uniforms.h"
#undef __S

#define __S text
#include "generated/text_uniforms.h"
#undef __S

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(3, &tile_vbo);  // Should also create sprite_vbo and text_vbo
	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tile_vertices), tile_vertices, GL_STATIC_DRAW);

	palette = new Palette({
		{
			{30, 40, 50},
			{50, 70, 90},
			{80, 110, 140},
			{120, 160, 200},
		}
	});

	v_width = width;
	v_height = height;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint framebuf;
	glGenTextures(1, &framebuf);
	glBindTexture(GL_TEXTURE_2D, framebuf);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuf, 0);
	framebuffer = new Texture(framebuf);

	sprite_attrs = (SpriteAttributes*)calloc(SPRITE_MAX, sizeof(SpriteAttributes));
	//glGenBuffers(1, &sprite_vbo); // done earlier
	glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SpriteAttributes) * SPRITE_MAX, sprite_attrs, GL_DYNAMIC_DRAW); // reserve GPU memory

	//glGenBuffers(1, &text_vbo); // done earlier
	glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphRenderData) * GLYPH_MAX, sprite_attrs, GL_DYNAMIC_DRAW); // reserve GPU memory

	temp_string_storage = (char*)malloc(STRING_STORAGE_SIZE);
	string_storage_next = temp_string_storage;
	print_later_ws_start = (GlyphPrintData*)malloc(sizeof(GlyphPrintData) * PRINT_CMD_WS_MAX);
	print_later_ws = print_later_ws_start;
	print_later_ss_start = (GlyphPrintData*)malloc(sizeof(GlyphPrintData) * PRINT_CMD_SS_MAX);
	print_later_ss = print_later_ss_start;

	ui_camera = glm::ortho(0.f, (float) width, 0.f, (float) height, 1024.f, -1024.f);
	world_camera = glm::ortho(0.f, (float) width, 0.f, (float) height, 128.f, -128.f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void Renderer::draw_frame(float fps, bool show_fps, bool show_console, bool show_cursor) {
#ifndef NDEBUG
	float start_time = glfwGetTime();
#endif

	u32 chunk_order[CHUNK_MAX];
	u32 clen = _sort_chunks(chunk_order);

	u32 sprite_order[SPRITE_MAX];
	u32 slen = _sort_sprites(sprite_order);
	// prepare all the sprite attributes for sending to the GPU
	for (u32 i = 0; i < slen; i++) {
		auto it = sprite_order[i];
		sprite_attrs[i] = sprites[it].attrs;
	}

	// Prepare for drawing
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, v_width, v_height);
	glClearColor(0.0f, 0.1f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(vao);
	// Draw tilemaps and sprites

	enum { NONE = 0, TILECHUNK, SPRITE } current_shader = NONE;
	u32 ci = 0, si = 0;

	while (ci < clen || si < slen) {
		if (si >= slen // no more sprites to draw
			|| ci < clen && chunks[chunk_order[ci]].layer <= sprite_attrs[si].layer) { // or this chunk is on the same layer or below as the next sprite
			if (current_shader != TILECHUNK) {
				tile_shader.use();
				tile_shader.set(tile_slots.palette, palette->bind(1));
				tile_shader.setCamera(world_camera);

				glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
				glEnableVertexAttribArray(0);
				glDisableVertexAttribArray(3);
				glDisableVertexAttribArray(4);

				current_shader = TILECHUNK;
			}
			auto& chunk = chunks[chunk_order[ci]];

			tile_shader.set(tile_slots.tileset, chunk.chunk->tileset->bind(0));
			tile_shader.set(tile_slots.chunk_size, (int)chunk.chunk->width);
			tile_shader.set(tile_slots.offset, chunk.x, chunk.y);
			// Convention: Display Color 0 on layers 0 and below.
			tile_shader.set(tile_slots.transparent_color0, chunk.layer > 0);

			glBindBuffer(GL_ARRAY_BUFFER, chunk.chunk->vbo);
			glVertexAttribIPointer(1, 1, GL_INT, sizeof(Tile), (void*)offsetof(Tile, tile));
			glEnableVertexAttribArray(1);
			glVertexAttribDivisor(1, 1);
			glVertexAttribIPointer(2, 1, GL_INT, sizeof(Tile), (void*)offsetof(Tile, cset));
			glEnableVertexAttribArray(2);
			glVertexAttribDivisor(2, 1);

			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, chunk.chunk->width * chunk.chunk->height);

			ci++;
		}
		else {
			if (current_shader != SPRITE) {
				sprite_shader.use();
				sprite_shader.set(sprite_slots.palette, palette->bind(1));
				sprite_shader.setCamera(world_camera);

				glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
				glVertexAttribIPointer(1, 4, GL_INT, sizeof(SpriteAttributes), (void*)offsetof(SpriteAttributes, src_x));
				glEnableVertexAttribArray(1);
				glVertexAttribDivisor(1, 1);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteAttributes), (void*)offsetof(SpriteAttributes, x));
				glEnableVertexAttribArray(2);
				glVertexAttribDivisor(2, 1);
				glVertexAttribIPointer(3, 1, GL_INT, sizeof(SpriteAttributes), (void*)offsetof(SpriteAttributes, layer));
				glEnableVertexAttribArray(3);
				glVertexAttribDivisor(3, 1);
				glVertexAttribIPointer(4, 1, GL_INT, sizeof(SpriteAttributes), (void*)offsetof(SpriteAttributes, flags));
				glEnableVertexAttribArray(4);
				glVertexAttribDivisor(4, 1);

				current_shader = SPRITE;
			}
			// Figure out how many sprites in a row can be drawn
			auto ss = sprites[sprite_order[si]].spritesheet;
			u32 lookahead;
			for (lookahead = si + 1; lookahead < slen; lookahead++) {
				// scan until we find a sprite with either a different spritesheet or one that would go over the next chunk
				if (ci < clen && chunks[chunk_order[ci]].layer <= sprite_attrs[lookahead].layer
					|| sprites[sprite_order[lookahead]].spritesheet != ss) break;
			}

			sprite_shader.set(sprite_slots.spritesheet, const_cast<Texture*>(ss)->bind(0));

			// sprite_vbo should already be bound at this point.
#ifndef NDEBUG
			{
				GLint bound_vbo;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound_vbo);
				assert(bound_vbo == sprite_vbo);
			}
#endif
			// TODO: determine if it's worth it to upgrade to OpenGL 4.2 for glDrawArraysInstancedBaseInstance
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SpriteAttributes) * (lookahead - si), &sprite_attrs[si]);

			glBindVertexArray(vao);
			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, lookahead - si);

			si = lookahead;
		}
	}


	// Print ALL the text!
	GlyphRenderData glyph_buffer[GLYPH_MAX];

	if ( // If there is text to print, get the text shader warmed up.
		show_fps
		|| print_later_ws > print_later_ws_start
		|| print_later_ss > print_later_ss_start
	) {
		text_shader.use();
		//text_shader.set(text_slots.layer, 500.f);

		glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);

		glVertexAttribIPointer(1, 4, GL_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, src_x));
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, x));
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1);
		glVertexAttribIPointer(3, 1, GL_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, rgba));
		glEnableVertexAttribArray(3);
		glVertexAttribDivisor(3, 1);
	}

	if (print_later_ws > print_later_ws_start) {
		text_shader.setCamera(world_camera);
		for (auto it = print_later_ws_start; it < print_later_ws; it++) {
			int n_glyphs = print_glyphs(it->font, glyph_buffer, GLYPH_MAX, it->text, it->x, it->y);
			text_shader.set(text_slots.glyph_atlas, bind_font_glyph_atlas(*it->font, 0));

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphRenderData) * n_glyphs, glyph_buffer);
			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_glyphs);
		}
	}

	if (print_later_ss > print_later_ss_start) {
		text_shader.setCamera(ui_camera);
		for (auto it = print_later_ss_start; it < print_later_ss; it++) {
			int n_glyphs = print_glyphs(it->font, glyph_buffer, GLYPH_MAX, it->text, it->x, it->y);
			text_shader.set(text_slots.glyph_atlas, bind_font_glyph_atlas(*it->font, 0));

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphRenderData) * n_glyphs, glyph_buffer);
			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_glyphs);
		}
	}

	// Print FPS
	if (show_fps) {
		char fps_msg[32];
		u32 fps_color = fps > 55.f ? 0x00FF00 : fps > 25.f ? 0xFFFF00 : 0xFF0000;
		snprintf(fps_msg, sizeof(fps_msg), "#c[%06x]%d FPS", fps_color, (int)fps);
		int n_glyphs = print_glyphs(&simple_font, glyph_buffer, 16, fps_msg, 1, 1);
		assert(n_glyphs >= 0);

		if (!(print_later_ss > print_later_ss_start)) {
			text_shader.setCamera(ui_camera);
		}
		text_shader.set(text_slots.glyph_atlas, bind_font_glyph_atlas(simple_font, 0));
		//text_shader.set(text_slots.layer, 500.f);

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphRenderData) * n_glyphs, glyph_buffer);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_glyphs);
	}

	if (show_console) {
		overlay_shader.use();

		glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		text_shader.use();
		//text_shader.set(text_slots.layer, 500.f);
		text_shader.setCamera(ui_camera);
		text_shader.set(text_slots.glyph_atlas, bind_font_glyph_atlas(simple_font, 0));

		glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
		glVertexAttribIPointer(1, 4, GL_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, src_x));
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, x));
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1);
		glVertexAttribIPointer(3, 1, GL_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, rgba));
		glEnableVertexAttribArray(3);
		glVertexAttribDivisor(3, 1);

		int n_glyphs = print_glyphs(&simple_font, glyph_buffer, GLYPH_MAX, get_console_line(show_cursor), CONSOLE_LINE_OFFSET_LEFT, v_height - CONSOLE_LINE_OFFSET_BOTTOM);

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphRenderData) * n_glyphs, glyph_buffer);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_glyphs);

		auto line_height = get_font_dimensions(simple_font).line_height;
		int scrollback_base = v_height - CONSOLE_LINE_OFFSET_BOTTOM - CONSOLE_LINE_SCROLLBACK_SPACING;
		int scrollback_max = (scrollback_base - SCROLLBACK_PADDING_TOP) / line_height;
		for (int i = 1; i < scrollback_max; i++) {
			const auto* sb_line = get_console_scrollback_line(i);
			if (sb_line == nullptr) break;
			n_glyphs = print_glyphs(&simple_font, glyph_buffer, GLYPH_MAX, sb_line, CONSOLE_LINE_OFFSET_LEFT, scrollback_base - (i * line_height));

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GlyphRenderData) * n_glyphs, glyph_buffer);
			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, n_glyphs);
		}
	}

	// virtual resolution scaling

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	scale_shader.use();
	scale_shader.set(scale_slots.virtual_screen, framebuffer->bind(0));

	// Determine the letterboxing ratio
	float scale_x = (float) screen_width / (float) v_width;
	float scale_y = (float) screen_height / (float) v_height;
	float max_scale = max(scale_x, scale_y);

	scale_shader.set(scale_slots.letterbox, glm::vec2(
		scale_y / max_scale, scale_x / max_scale
	));
	scale_shader.set(scale_slots.sharpness, max(scaling_sharpness, 0.f) * max_scale / 5.f);

	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

#ifndef NDEBUG
	float render_time = glfwGetTime() - start_time;
	if (render_time > 0.01)
	{
		printf("ALERT: Render took %.2fms this frame\n", render_time * 1000.f);
	}
#endif

	string_storage_next = temp_string_storage;
	print_later_ws = print_later_ws_start;
	print_later_ss = print_later_ss_start;

	glfwSwapBuffers(window);
}

bool Renderer::_print_text(Font* font, CoordinateSystem coords, float x, float y, const char* const format, va_list args) {
	if (coords == WORLD_SPACE && print_later_ws - print_later_ws_start >= PRINT_CMD_WS_MAX) {
		fprintf(stderr, "Out of world space text slots.\n");
		return false;
	}
	else if (coords == SCREEN_SPACE && print_later_ss - print_later_ss_start >= PRINT_CMD_SS_MAX) {
		fprintf(stderr, "Out of screen space text slots.\n");
		return false;
	}
	int text_memory_remaining = STRING_STORAGE_SIZE - (string_storage_next - temp_string_storage);
	// Check how much space is required
	auto text = string_storage_next;
	int len = vsnprintf(string_storage_next, text_memory_remaining, format, args);
	if (len < 0 || len + 1 > text_memory_remaining) {
		fprintf(stderr, "Not enough string storage memory.\n");
		return false;
	}
	string_storage_next += len + 2; // +1 to get to the '\0', +1 to get to the character after.
	if (coords == WORLD_SPACE) {
		*print_later_ws++ = { font, text, x, y };
	}
	else if (coords == SCREEN_SPACE) {
		*print_later_ss++ = { font, text, x, y };
	}
	return true;
}

#define _HANDOFF(FONT, COORDS) do{\
	va_list args;\
	va_start(args, format);\
	bool ret = _print_text((FONT), (COORDS), x, y, format, args);\
	va_end(args);\
	return ret;\
} while(0)

bool Renderer::print_text(Font* font, CoordinateSystem coords, float x, float y, const char* format, ...) {
	_HANDOFF(font, coords);
}

bool Renderer::print_text(CoordinateSystem coords, float x, float y, const char* format, ...) {
	_HANDOFF(&simple_font, coords);
}

bool Renderer::print_text(Font* font, float x, float y, const char* format, ...) {
	_HANDOFF(font, SCREEN_SPACE);
}

bool Renderer::print_text(float x, float y, const char* format, ...) {
	_HANDOFF(&simple_font, SCREEN_SPACE);
}

#undef _HANDOFF

u32 Renderer::_sort_chunks(u32* buffer) {
	auto len = chunks.fill_index(buffer, CHUNK_MAX);
	std::sort(buffer, buffer + len, [&](int left, int right) {
		auto l = chunks[left].layer;
		auto r = chunks[right].layer;
		if (l == r) return left < right; // whee, cheap sort stability!
		else return l < r;
	});
	return len;
}

u32 Renderer::_sort_sprites(u32* buffer) {
	auto len = sprites.fill_index(buffer, SPRITE_MAX);
	std::sort(buffer, buffer + len, [&](int left, int right) {
		auto l = sprites[left].attrs.layer;
		auto r = sprites[right].attrs.layer;
		if (l == r) {
			auto lt = (intptr_t) sprites[left].spritesheet;
			auto rt = (intptr_t) sprites[right].spritesheet;
			if (lt == rt) return left < right; // whee, cheap sort stability!
			else return lt < rt;
		}
		else return l < r;
	});
	return len;
}

ChunkID Renderer::add_chunk(const TileChunk* const chunk, float x, float y, i32 layer) {
	return chunks.add(ChunkEntry{chunk, x, y, layer});
}

bool Renderer::remove_chunk(const ChunkID id) {
	return chunks.remove(id);
}

SpriteID  Renderer::add_sprite(
	Texture* const spritesheet,
	float x, float y,
	i32 layer,
	i32 src_x, i32 src_y, i32 src_w, i32 src_h,
	u8 cset,
	u32 flip,
	float r, float g, float b, float a,
	bool show_color0
) {
	u32 red   = (u32)((1.f - clamp(r)) * 31.f) << 24;
	u32 green = (u32)((1.f - clamp(g)) * 31.f) << 19;
	u32 blue  = (u32)((1.f - clamp(b)) * 31.f) << 14;
	u32 alpha = (u32)((1.f - clamp(a)) * 31.f) << 9;

	return sprites.add(Sprite{
		spritesheet,
		{
			src_x, src_y, src_w, src_h,
			x, y,
			layer,
			cset | flip | (show_color0 ? SHOW_COLOR0 : 0) | red | green | blue | alpha
		}
	});
}

bool Renderer::remove_sprite(const SpriteID id) {
	return sprites.remove(id);
}


TileChunk::TileChunk(Texture* const tileset, Tile* const tilemap, u32 width, u32 height):
	tileset(tileset),
	tilemap(tilemap),
	width(width),
	height(height),
	vbo(0)
{
	glGenBuffers(1, const_cast<GLuint*>(&vbo));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tilemap) * width * height, tilemap, GL_DYNAMIC_DRAW);
}


TileChunk::~TileChunk() {
	glDeleteBuffers(1, &vbo);
}

void TileChunk::sync() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tilemap) * width * height, tilemap);
}

u32 rotateCCW(u32 tile) {
	return vflip(transpose(tile));
}

u32 rotateCW(u32 tile) {
	return transpose(vflip(tile));
}

u32 hflip(u32 tile) {
	return tile ^ ((tile & DFLIP)? VFLIP : HFLIP);
}

u32 vflip(u32 tile) {
	return tile ^ ((tile & DFLIP)? HFLIP : VFLIP);
}

u32 transpose(u32 tile) {
	return tile ^ DFLIP;
}

u32 filter_tile(u32 cset, float r, float g, float b) {
	u32 red = (u32) ((1.f - clamp(r)) * 255.f) << 24;
	u32 green = (u32) ((1.f - clamp(g)) * 255.f) << 16;
	u32 blue = (u32) ((1.f - clamp(b)) * 255.f) << 8;
	return red | green | blue | (cset & 0xFF);
}

