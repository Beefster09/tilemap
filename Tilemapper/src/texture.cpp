#include <glad/glad.h>
#include <glfw3.h>
#include <cstdio>
#include <cassert>

#include "texture.h"
#include "stb_image.h"

static uint32_t bindingNumber = 0;

struct TextureBindingEntry {
	Texture* tex;
	uint32_t lastBind; // Number that marks this as the nth binding
	uint32_t bindCount; // Number of times this texture has been bound
};

static TextureBindingEntry boundTextures[16] = {
	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},

	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},

	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},

	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},
	{nullptr, 0, 0},
};

struct Texture {
	GLuint tex_handle;
	GLenum gl_type = GL_TEXTURE_2D;
	int bound_slot = TEX_UNBOUND;

	Texture(GLuint tex, GLenum type) {
		tex_handle = tex;
		gl_type = type;
	}

	int bind(int slot) {
		if (bound_slot >= 0) {
			boundTextures[bound_slot].lastBind = bindingNumber++;
			boundTextures[bound_slot].bindCount++;
			return bound_slot;
		}
		if (slot == TEX_AUTO) {
			uint32_t oldest = UINT32_MAX;
			for (int i = 0; i < 16; i++) {
				if (boundTextures[i].tex) {
					if (boundTextures[i].lastBind < oldest) {
						slot = i;
						oldest = boundTextures[i].lastBind;
					}
				}
				else {
					slot = i;
					break;
				}
			}
		}
		if (boundTextures[slot].tex) {
			boundTextures[slot].tex->evict();
		}
		assert(slot >= 0 && slot < 16);
		bound_slot = slot;
		boundTextures[slot] = { this, bindingNumber++, 1 };
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(gl_type, tex_handle);
		return slot;
	}

	void evict() {
		boundTextures[bound_slot] = { nullptr, 0, 0 };
		bound_slot = TEX_UNBOUND;
	}
};

int bind(Texture* tex, int slot) {
	return tex->bind(slot);
}

Texture* make_texture(GLuint tex, GLenum type) {
	return new Texture(tex, type);
}

struct Tileset {
	Texture tex;
};

Tileset* load_tileset(const char* image_file, int tile_size, int offset_x, int offset_y, int spacing_x, int spacing_y) {
	stbi_set_flip_vertically_on_load(false);
	int width, height, n_channels;
	unsigned char *image_data = stbi_load(image_file, &width, &height, &n_channels, 0);

	if (image_data) {
		int tiles_horiz = (width - offset_x + spacing_x) / (tile_size + spacing_x);
		int tiles_vert = (height - offset_y + spacing_x) / (tile_size + spacing_y);
		unsigned char* tile_data = new unsigned char[tiles_horiz * tiles_vert * width * height];
		unsigned char* data_ptr = tile_data;
		for (int top = offset_y; top + tile_size <= height; top += tile_size + spacing_y) { // row major tile iteration
			for (int left = offset_x; left + tile_size <= width; left += tile_size + spacing_x) {
				// for each tile...
				for (int y = 0; y < tile_size; y++) {
					for (int x = 0; x < tile_size; x++) {
						*data_ptr++ = image_data[
							((top + y) * width + (left + x)) * n_channels
						];
						/*if (data_ptr[-1]) {
							printf("%d ", data_ptr[-1] % 4);
						}
						else { printf(". "); }*/
					}
					//printf("\n");
				}
				//printf("\n");
			}
		}

		GLuint tex_handle;
		glGenTextures(1, &tex_handle);
		glBindTexture(GL_TEXTURE_2D_ARRAY, tex_handle);

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8UI, tile_size, tile_size, tiles_horiz * tiles_vert, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, tile_data);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		delete[] tile_data;
		stbi_image_free(image_data);
		return new Tileset{
			Texture(tex_handle, GL_TEXTURE_2D_ARRAY)
		};
	}
	else {
		printf("Unable to load texture '%s'\n", image_file);
		return nullptr;
	}
}

int bind(Tileset* ts, int slot) {
	return ts->tex.bind(slot);
}

struct Spritesheet {
	Texture tex;
};

Spritesheet* load_spritesheet(const char* image_file) {
	stbi_set_flip_vertically_on_load(false);
	int width, height, n_channels;
	unsigned char *image_data = stbi_load(image_file, &width, &height, &n_channels, 0);

	if (image_data) {
		unsigned char* spritesheet_data = new unsigned char[width * height];
		for (int i = 0; i < width * height; i++) {
			spritesheet_data[i] = image_data[i * n_channels];
		}
		GLuint tex_handle;
		glGenTextures(1, &tex_handle);
		glBindTexture(GL_TEXTURE_RECTANGLE, tex_handle);

		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, spritesheet_data);

		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		delete[] spritesheet_data;
		stbi_image_free(image_data);
		return new Spritesheet{
			Texture(tex_handle, GL_TEXTURE_RECTANGLE)
		};
	}
	else {
		printf("Unable to load texture '%s'\n", image_file);
		return nullptr;
	}
}

int bind(Spritesheet* ss, int slot) {
	return ss->tex.bind(slot);
}

struct Palette {
	Texture tex;
	Color* color_data;
	GLuint color_buffer;
	int n_csets;
	int cset_size;
};

Palette* make_palette(std::initializer_list<std::initializer_list<Color>> color_data) {
	auto csets = color_data.size();
	auto cset_size = color_data.begin()->size();
	auto colors = alloc(Color, csets * cset_size);
	int i = 0;
	auto r_end = color_data.end();
	for (auto row = color_data.begin(); row != r_end; row++) {
		assert(row->size() == cset_size && "Initializer list for palette is jagged!");
		auto c_end = row->end();
		for (auto cell = row->begin(); cell != c_end; cell++) {
			colors[i++] = *cell;
		}
	}

	GLuint color_buffer;
	glGenBuffers(1, &color_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, color_buffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(Color) * csets * cset_size, colors, GL_DYNAMIC_DRAW);

	GLuint tex_handle;
	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_BUFFER, tex_handle);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, color_buffer);

	return new Palette{
		Texture(tex_handle, GL_TEXTURE_BUFFER),
		colors,
		color_buffer,
		(int) csets,
		(int) cset_size
	};
}

int bind(Palette* p, int slot) {
	return p->tex.bind(slot);
}

void sync(Palette* p) {
	glBindBuffer(GL_TEXTURE_BUFFER, p->color_buffer);
	glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(Color) * p->n_csets * p->cset_size, p->color_data);
}
