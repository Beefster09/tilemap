#pragma once

#include "shader.h"

class Renderer {
private:
	GLFWwindow* const window;
	int v_width, v_height; // virtual resolution

	GLuint vao, tile_vbo, tilemap_vbo, fbo;
	Shader tile_shader, scale_shader;

	int tileset_slot, palette_slot, chunk_size_slot, tile_size_slot, flags_slot;
	int virtual_screen_slot, sharpness_slot, letterbox_slot;

	Texture* tileset;
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

constexpr uint32_t HFLIP = 0x00080000;
constexpr uint32_t VFLIP = 0x00040000;
constexpr uint32_t DFLIP = 0x00020000;

uint32_t rotateCW(uint32_t tile = 0);
uint32_t rotateCCW(uint32_t tile = 0);
uint32_t hflip(uint32_t tile = 0);
uint32_t vflip(uint32_t tile = 0);
uint32_t transpose(uint32_t tile = 0);

uint32_t dither(uint32_t tile, uint32_t x_mult, uint32_t y_mult, uint32_t mod = 2, bool parity = false);

// tile render modifiers

uint32_t filter(uint32_t cset, float r, float g, float b);

