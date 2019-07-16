#pragma once

#include "shader.h"
#include "table.h"
#include "text.h"

class Renderer;
struct GlyphPrintData;

enum CoordinateSystem {
	WORLD_SPACE,
	SCREEN_SPACE
};

struct Tile {
	u32 tile;
	u32 cset;
};

class TileChunk {
	Texture* const tileset;
	Tile* const tilemap;
	const u32 width;
	const u32 height;
	const GLuint vbo;

public:
	TileChunk(Texture* const tileset, Tile* const tilemap, u32 width, u32 height);
	~TileChunk();

	inline Tile& at(u32 row, u32 col) {
		assert(row < height && col < width);
		return tilemap[row * width + col];
	}

	void sync();

friend class Renderer;
};

struct ChunkEntry {
	const TileChunk* chunk = nullptr;
	float x, y;
	i32 layer;
	float alpha;
};

struct SpriteAttributes {
	i32 src_x, src_y, src_w, src_h;
	float x, y;
	i32 layer;
	u32 flags;
};

struct Sprite {
	Texture* spritesheet;
	SpriteAttributes attrs;
};

typedef Table<ChunkEntry>::Handle ChunkID;
typedef Table<Sprite>::Handle SpriteID;

class Renderer {
private:
	GLFWwindow* const window;
	int v_width, v_height; // virtual resolution

	GLuint vao, fbo, tile_vbo, sprite_vbo, text_vbo;
	Shader tile_shader, scale_shader, sprite_shader, text_shader;

#define __SLOT(VAR) int VAR;
	struct {
#include "generated/tilechunk_uniforms.h"
	} tile_slots;
	struct {
#include "generated/scale_uniforms.h"
	} scale_slots;
	struct {
#include "generated/sprite_uniforms.h"
	} sprite_slots;
	struct {
#include "generated/text_uniforms.h"
	} text_slots;
#undef __SLOT

	Palette* palette;

	Texture* framebuffer;

	Table<ChunkEntry> chunks;
	Table<Sprite> sprites;

	SpriteAttributes* sprite_attrs;
	//GlyphRenderData* text_render_buffer;
	GlyphPrintData* print_later_ws_start;
	GlyphPrintData* print_later_ws;
	GlyphPrintData* print_later_ss_start;
	GlyphPrintData* print_later_ss;

	glm::mat4 world_camera, ui_camera;
	float scaling_sharpness = 2.f;

	char* temp_string_storage;
	char* string_storage_next;

	u32 _sort_chunks(u32 * buffer);
	u32 _sort_sprites(u32 * buffer);

	bool _print_text(Font* font, CoordinateSystem coords, float x, float y, const char* format, va_list args);
public:
	Renderer(GLFWwindow* window, int width, int height);
	void draw_frame(float fps, bool show_fps);


	inline void set_sharpness(float sharpness) {
		scaling_sharpness = max(sharpness, 0.f);
	}
	inline float get_sharpness() {
		return scaling_sharpness;
	}

	ChunkID add_chunk(const TileChunk* const chunk, float x, float y, i32 layer);
	bool remove_chunk(const ChunkID id);

	SpriteID add_sprite(
		Texture* const spritesheet,
		float x, float y,
		i32 layer,
		i32 src_x, i32 src_y, i32 src_w, i32 src_h,
		u8 cset,
		u32 flip = 0,
		float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f,
		bool show_color0 = false
	);
	bool remove_sprite(const SpriteID id);

	bool print_text(Font* font, CoordinateSystem coords, float x, float y, const char* format, ...);
	bool print_text(CoordinateSystem coords, float x, float y, const char* format, ...);
	bool print_text(Font* font, float x, float y, const char* format, ...);
	bool print_text(float x, float y, const char* format, ...);
};

// tile modifiers

constexpr u32 HFLIP = 0x80000000;
constexpr u32 VFLIP = 0x40000000;
constexpr u32 DFLIP = 0x20000000;

u32 rotateCW(u32 tile = 0);
u32 rotateCCW(u32 tile = 0);
u32 hflip(u32 tile = 0);
u32 vflip(u32 tile = 0);
u32 transpose(u32 tile = 0);

// tile render modifiers

u32 filter_tile(u32 cset, float r, float g, float b);

constexpr u32 SHOW_COLOR0 = 0x100;

u32 filter_sprite(u8 cset, float r, float g, float b);
