#pragma once

#include "shader.h"

class TileChunk {
	Texture* const tileset;
	u32* const tilemap;
	const u32 width;
	const u32 height;
	const GLuint vbo;

public:
	TileChunk(Texture* tileset, u32* tilemap, u32 width, u32 height);
	~TileChunk();

	inline u32& at(u32 row, u32 col) {
		assert(row < height && col < width);
		return tilemap[row * width + col];
	}

	void sync();

friend class Renderer;
};

class Renderer {
private:
	GLFWwindow* const window;
	int v_width, v_height; // virtual resolution

	GLuint vao, tile_vbo, fbo;
	Shader tile_shader, scale_shader;

	int tileset_slot, palette_slot, chunk_size_slot, tile_size_slot, flags_slot;
	int virtual_screen_slot, sharpness_slot, letterbox_slot;

	Palette* palette;

	Texture* framebuffer;

	glm::mat4 camera;
	float scaling_sharpness = 2.f;

public:
	Renderer(GLFWwindow* window, int width, int height);
	void draw_frame();

	inline void set_sharpness(float sharpness) {
		scaling_sharpness = max(sharpness, 0.f);
	}
	inline float get_sharpness() {
		return scaling_sharpness;
	}
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

