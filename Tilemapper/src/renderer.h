#pragma once

#include "shader.h"

class Renderer {
private:
	GLFWwindow* const window;
	int v_width, v_height; // virtual resolution

	GLuint vao, tile_vbo, tilemap_vbo, fbo;
	Shader tile_shader, scale_shader;

	int tileset_slot, palette_slot, chunk_size_slot;
	int virtual_screen_slot;

	Texture* tileset;
	Palette* palette;

	Texture* framebuffer;

	glm::mat4 camera;

public:
	Renderer(GLFWwindow* window, int width, int height);
	void draw_frame();
};

// tile flip masks

constexpr unsigned int HFLIP = 0x80000000;
constexpr unsigned int VFLIP = 0x40000000;
constexpr unsigned int DFLIP = 0x20000000;

inline unsigned int rotateCW(unsigned int tile = 0);
inline unsigned int rotateCCW(unsigned int tile = 0);
inline unsigned int hflip(unsigned int tile = 0);
inline unsigned int vflip(unsigned int tile = 0);
inline unsigned int transpose(unsigned int tile = 0);

