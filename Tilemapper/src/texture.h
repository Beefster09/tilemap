#pragma once

#include <vector>

#include "common.h"

constexpr int TEX_UNBOUND = -1;
constexpr int TEX_AUTO = -2;

struct Texture;
Texture* make_texture(GLuint tex, GLenum type);
int bind(Texture* tex, int slot = TEX_AUTO);

struct Tileset;
Tileset* load_tileset(const char* image_file, int tile_size, int offset_x = 0, int offset_y = 0, int spacing_x = 0, int spacing_y = 0);
int bind(Tileset* tileset, int slot = TEX_AUTO);

struct Spritesheet;
Spritesheet* load_spritesheet(const char* image_file);
int bind(Spritesheet* spritesheet, int slot = TEX_AUTO);

struct Color {
	u8 r, g, b;
	u8 a = 255;
};

struct Palette;
Palette* make_palette(int csets, int cset_size);
Palette* make_palette(std::initializer_list<std::initializer_list<Color>> color_data);
Color get_color(Palette* palette, int cset, int index);
void set_color(Palette* palette, int cset, int index, Color color);
const Color* get_cset(Palette* palette, int cset);
void set_cset(Palette* palette, int cset, const Color* colors);
int n_csets(Palette* palette);
int cset_size(Palette* palette);
int bind(Palette* palette, int slot = TEX_AUTO);
void sync(Palette* palette);
