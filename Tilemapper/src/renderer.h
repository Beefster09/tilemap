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
};

class Sprite {
	Texture* spritesheet;
	u32 src_x, src_y, src_w, src_h;
	float x, y;
	i32 layer;
	float rotation;
	union {
		u8 color[4];
		struct {u8 red, green, blue, alpha;};
	};
	u8 cset;
};

typedef Table<ChunkEntry>::Handle ChunkID;
typedef Table<Sprite>::Handle SpriteID;

class Renderer {
private:
	GLFWwindow* const window;
	int v_width, v_height; // virtual resolution

	GLuint vao, tile_vbo, fbo;
	Shader tile_shader, scale_shader;

#define __SLOT(VAR) int VAR ## _slot;
#include "tilechunk_uniforms__generated.h"
#include "scale_uniforms__generated.h"
#undef __SLOT

	Palette* palette;

	Texture* framebuffer;

	//std::vector<ChunkEntry> chunks;
	Table<ChunkEntry> chunks;
	std::vector<int> chunk_order;

	Table<Sprite> sprites;
	std::vector<int> sprite_order;

	glm::mat4 camera;
	float scaling_sharpness = 2.f;

	void _sort_chunks();

public:
	Renderer(GLFWwindow* window, int width, int height);
	void draw_frame();

	inline void set_sharpness(float sharpness) {
		scaling_sharpness = max(sharpness, 0.f);
	}
	inline float get_sharpness() {
		return scaling_sharpness;
	}

	ChunkID add_chunk(const TileChunk* const chunk, float x, float y, i32 layer);
	bool remove_chunk(const ChunkID id);
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

