
#include <algorithm>

#include <glad/glad.h>
#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer.h"

constexpr int CHUNK_MAX = 64;
constexpr int SANE_SPRITE_MAX = 512;

const float tile_vertices[] = {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f,
};

extern int screen_width, screen_height;

#ifdef NO_EMBED_SHADERS

#define COMPILE_SHADER(V, F) compileShaderFromFiles((V), (F))
#define TILECHUNK_VERT_SHADER = "shaders/tilechunk.vert"
#define TILECHUNK_VERT_SHADER = "shaders/tilechunk.frag"
#define SCALE_VERT_SHADER = "shaders/scale.vert"
#define SCALE_VERT_SHADER = "shaders/scale.frag"

#define TILECHUNK_VERT_SHADER__SRC = "tilechunk.vert"
#define TILECHUNK_VERT_SHADER__SRC = "tilechunk.frag"
#define SCALE_VERT_SHADER__SRC = "scale.vert"
#define SCALE_VERT_SHADER__SRC = "scale.frag"

#else

#define COMPILE_SHADER(V, F) compileShader((V), (F))
#include "shaders__generated.h"

#endif

#define __SHADER(S) COMPILE_SHADER(S ## _VERT_SHADER, S ## _FRAG_SHADER), S ## _VERT_SHADER__SRC, S ## _FRAG_SHADER__SRC
#define __SHADER2(V, F) COMPILE_SHADER(V ## _VERT_SHADER, F ## _FRAG_SHADER), V ## _VERT_SHADER__SRC, F ## _FRAG_SHADER__SRC


#define __SLOT(VAR) (VAR ## _slot) = __S.getSlot(#VAR)
Renderer::Renderer(GLFWwindow* window, int width, int height):
	window(window),
	tile_shader(__SHADER(TILECHUNK)),
	scale_shader(__SHADER(SCALE)),
	chunks(CHUNK_MAX),
	sprites(SANE_SPRITE_MAX)
{
#define __S tile_shader
#include "tilechunk_uniforms__generated.h"
#undef __S

#define __S scale_shader
#include "scale_uniforms__generated.h"

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &tile_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tile_vertices), tile_vertices, GL_STATIC_DRAW);

	palette = new Palette({
		{
			{30, 40, 50},
			{50, 70, 90},
			{80, 110, 140},
			{120, 160, 200},
		}
	});

	v_width = width;
	v_height = height;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint framebuf;
	glGenTextures(1, &framebuf);
	glBindTexture(GL_TEXTURE_2D, framebuf);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuf, 0);
	framebuffer = new Texture(framebuf);

	camera = glm::ortho(0.f, (float) width, 0.f, (float) height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void Renderer::draw_frame(float fps, bool show_fps) {
#ifndef NDEBUG
	float start_time = glfwGetTime();
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, v_width, v_height);
	glClearColor(0.0f, 0.1f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw tilemaps

	tile_shader.use();
	tile_shader.set(palette_slot, palette->bind(1));
	tile_shader.setUint(flags_slot, 1);
	tile_shader.setTransform(glm::mat4(1.f));
	tile_shader.setCamera(camera);

	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glEnableVertexAttribArray(0);

	u32 chunk_order[CHUNK_MAX];
	u32 len = _sort_chunks(chunk_order);
	for (u32 i = 0; i < len; i++) {
		auto it = chunk_order[i];
		auto& chunk = chunks[it].chunk;
		tile_shader.set(tileset_slot, chunk->tileset->bind(0));
		tile_shader.set(chunk_size_slot, (int) chunk->width);
		tile_shader.set(offset_slot, chunks[it].x, chunks[it].y);

		glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
		glVertexAttribIPointer(1, 1, GL_INT, sizeof(Tile), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);
		glVertexAttribIPointer(2, 1, GL_INT, sizeof(Tile), (void*) sizeof(Tile::tile));
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1);

		glBindVertexArray(vao);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, chunk->width * chunk->height);
	}

	// virtual resolution scaling

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	scale_shader.use();
	scale_shader.set(virtual_screen_slot, framebuffer->bind(0));

	// Determine the letterboxing ratio
	float scale_x = (float) screen_width / (float) v_width;
	float scale_y = (float) screen_height / (float) v_height;
	float max_scale = max(scale_x, scale_y);

	scale_shader.set(letterbox_slot, glm::vec2(
		scale_y / max_scale, scale_x / max_scale
	));
	scale_shader.set(sharpness_slot, max(scaling_sharpness, 0.f) * max_scale / 5.f);

	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

#ifndef NDEBUG
	float render_time = glfwGetTime() - start_time;
	if (render_time > 0.016666f)
	{
		printf("ALERT: Render took %.2fms this frame\n", render_time * 1000.f);
	}
#endif

	glfwSwapBuffers(window);
}

u32 Renderer::_sort_chunks(u32* buffer) {
	auto len = chunks.fill_index(buffer, CHUNK_MAX);
	std::sort(buffer, buffer + len, [&](int left, int right) {
		auto l = chunks[left].layer;
		auto r = chunks[right].layer;
		if (l == r) return left < right; // whee, cheap sort stability!
		else return l < r;
	});
	return len;
}

ChunkID Renderer::add_chunk(const TileChunk* const chunk, float x, float y, i32 layer) {
	return chunks.add(ChunkEntry{chunk, x, y, layer});
}

bool Renderer::remove_chunk(const ChunkID id) {
	return chunks.remove(id);
}


TileChunk::TileChunk(Texture* const tileset, Tile* const tilemap, u32 width, u32 height):
	tileset(tileset),
	tilemap(tilemap),
	width(width),
	height(height),
	vbo(0)
{
	glGenBuffers(1, const_cast<GLuint*>(&vbo));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tilemap) * width * height, tilemap, GL_DYNAMIC_DRAW);
}


TileChunk::~TileChunk() {
	glDeleteBuffers(1, &vbo);
}

void TileChunk::sync() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tilemap) * width * height, tilemap);
}

u32 rotateCCW(u32 tile) {
	return vflip(transpose(tile));
}

u32 rotateCW(u32 tile) {
	return transpose(vflip(tile));
}

u32 hflip(u32 tile) {
	return tile ^ ((tile & DFLIP) ? VFLIP : HFLIP);
}

u32 vflip(u32 tile) {
	return tile ^ ((tile & DFLIP)? HFLIP : VFLIP);
}

u32 transpose(u32 tile) {
	return tile ^ DFLIP;
}

u32 dither(u32 tile, u32 x_mult, u32 y_mult, u32 mod, u32 phase, bool parity) {
	assert(x_mult < 4 && y_mult < 4 && mod <= 8 && mod > 0 && phase < 8);
	return (x_mult << 30) | (y_mult << 28) | ((mod - 1) << 25) | (phase << 22) | (parity * 0x00200000u) | (tile & 0x000FFFFF);
}

u32 filter(u32 cset, float r, float g, float b) {
	u32 red = (u32) ((1.f - clamp(r)) * 255) << 24;
	u32 green = (u32) ((1.f - clamp(g)) * 255) << 16;
	u32 blue = (u32) ((1.f - clamp(b)) * 255) << 8;
	return red | green | blue | (cset & 0x00003FFF);
}

