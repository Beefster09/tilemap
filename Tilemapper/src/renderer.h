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

// tile flip masks

constexpr unsigned int HFLIP = 0x80000000;
constexpr unsigned int VFLIP = 0x40000000;
constexpr unsigned int DFLIP = 0x20000000;

unsigned int rotateCW(unsigned int tile = 0);
unsigned int rotateCCW(unsigned int tile = 0);
unsigned int hflip(unsigned int tile = 0);
unsigned int vflip(unsigned int tile = 0);
unsigned int transpose(unsigned int tile = 0);

// tile render modifiers

constexpr unsigned int DITHER = 0x8000;
constexpr unsigned int DITHER_PARITY = 0x4000;

unsigned int filter(unsigned int cset, float r, float g, float b);

