#pragma once

#include "shader.h"
#include "table.h"

class Renderer;

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
	u32 src_x, src_y, src_w, src_h;
	float x, y, px, py;
	i32 layer;
	float rotation;
	u32 cset;
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

	GLuint vao, tile_vbo, fbo, sprite_vbo;
	Shader tile_shader, scale_shader, sprite_shader;

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
#undef __SLOT

	Palette* palette;

	Texture* framebuffer;

	Table<ChunkEntry> chunks;
	Table<Sprite> sprites;
	SpriteAttributes* sprite_attrs;

	glm::mat4 camera;
	float scaling_sharpness = 2.f;
	float last_frame_time = 1.f;

	u32 _sort_chunks(u32 * buffer);
	u32 _sort_sprites(u32 * buffer);

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

	ChunkID add_chunk(const TileChunk* const chunk, float x, float y, i32 layer);
};

// tile modifiers

constexpr u32 HFLIP = 0x00080000;
constexpr u32 VFLIP = 0x00040000;
constexpr u32 DFLIP = 0x00020000;

u32 rotateCW(u32 tile = 0);
u32 rotateCCW(u32 tile = 0);
u32 hflip(u32 tile = 0);
u32 vflip(u32 tile = 0);
u32 transpose(u32 tile = 0);

u32 dither(u32 tile, u32 x_mult, u32 y_mult, u32 mod = 2, u32 phase = 0, bool parity = false);

// tile render modifiers

u32 filter(u32 cset, float r, float g, float b);

constexpr u32 CHUNK_FLAG_SHOW_COLOR0 = 0x1;
