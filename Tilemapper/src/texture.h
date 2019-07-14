#pragma once

#include <vector>

#include "common.h"

constexpr int TEX_UNBOUND = -1;
constexpr int TEX_AUTO = -2;

class Texture {
private:
	GLuint tex_handle;
	GLenum type = GL_TEXTURE_2D;
	int bound_slot = TEX_UNBOUND;

	void evict();

public:
	//Texture(const char* file);
	Texture(GLuint tex, GLenum type = GL_TEXTURE_2D);
	Texture() = default;
	~Texture();
	
	int bind(int slot = TEX_AUTO);
};

Texture* load_tileset(const char* image_file, int tile_size, int offset_x = 0, int offset_y = 0, int spacing_x = 0, int spacing_y = 0);
Texture* load_spritesheet(const char* image_file);

struct Color {
	u8 r, g, b;
};

class Palette {
private:
	Texture tex;
	std::vector<Color> colors;
	const int csets;
	const int cset_size;

public:
	//Palette(int csets, int cset_size);
	Palette(std::initializer_list<std::initializer_list<Color>> color_data);

	void sync();
	inline int bind(int slot = TEX_AUTO) { return tex.bind(slot); }
};