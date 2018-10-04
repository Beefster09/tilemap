#include <glad/glad.h> 
#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer.h"

const float tile_vertices[] = {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 0.f,

	0.f, 1.f,
	1.f, 0.f,
	1.f, 1.f,
};

const uint32_t simple_tilemap[] = {
	2,       0, 2|HFLIP,       0, 2|DFLIP,       0, 2|HFLIP|DFLIP, 0,
	2|VFLIP, 0, 2|HFLIP|VFLIP, 0, 2|DFLIP|VFLIP, 0, 2|HFLIP|DFLIP|VFLIP, 0,
	2, 0, rotateCW(2), 0, rotateCW(rotateCW(2)), filter(0, 1.f, 0.5f, 0.f), rotateCCW(2), 0,
	2|VFLIP, 0, rotateCW(2|VFLIP), 0, rotateCW(rotateCW(2|VFLIP)), 0, rotateCCW(2|VFLIP), 0,
};

extern int screen_width, screen_height;

Renderer::Renderer(GLFWwindow* window, int width, int height): 
	window(window),
	tile_shader("shaders/tilechunk.vert", "shaders/tilechunk.frag"),
	scale_shader("shaders/scale.vert", "shaders/scale.frag")
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	logOpenGLErrors();

	glGenBuffers(1, &tile_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tile_vertices), tile_vertices, GL_STATIC_DRAW);
	logOpenGLErrors();

	uint32_t simple_tilemap[128];
	uint32_t *tile = simple_tilemap;
	for (int mulx = 0; mulx < 8; mulx++) {
		for (int muly = 0; muly < 8; muly++) {
			*tile++ = dither(2, mulx, muly, mulx + muly + 1, true);
			*tile++ = 0;
		}
	}

	glGenBuffers(1, &tilemap_vbo); // TODO: separate into Chunk class/struct
	glBindBuffer(GL_ARRAY_BUFFER, tilemap_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(simple_tilemap), simple_tilemap, GL_DYNAMIC_DRAW);
	logOpenGLErrors();

	tileset_slot = tile_shader.getSlot("tileset");
	palette_slot = tile_shader.getSlot("palette");
	chunk_size_slot = tile_shader.getSlot("chunk_size");
	tile_size_slot = tile_shader.getSlot("tile_size");
	flags_slot = tile_shader.getSlot("flags");

	tileset = load_tileset("assets/tileset24bit.png", 16);
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
	logOpenGLErrors();

	GLuint framebuf;
	glGenTextures(1, &framebuf);
	glBindTexture(GL_TEXTURE_2D, framebuf);
	logOpenGLErrors();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	logOpenGLErrors();

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuf, 0);
	logOpenGLErrors();
	framebuffer = new Texture(framebuf);

	virtual_screen_slot = scale_shader.getSlot("virtual_screen");
	sharpness_slot = scale_shader.getSlot("sharpness");
	letterbox_slot = scale_shader.getSlot("letterbox");

	camera = glm::ortho(0.f, (float) width, 0.f, (float) height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	logOpenGLErrors();
}

void Renderer::draw_frame() {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, v_width, v_height);
	glClearColor(0.0f, 0.1f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	logOpenGLErrors();

	// Draw tilemaps

	tile_shader.use();
	tile_shader.set(chunk_size_slot, 8);
	tile_shader.set(tileset_slot, tileset->bind(0));
	tile_shader.set(palette_slot, palette->bind(1));
	tile_shader.setUint(flags_slot, 1);
	tile_shader.setTransform(glm::mat4(1.f));
	tile_shader.setCamera(camera);

	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, tilemap_vbo);
	glVertexAttribIPointer(1, 1, GL_INT, sizeof(int) * 2, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(int) * 2, (void*) sizeof(int));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);
	logOpenGLErrors();

	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 64);
	logOpenGLErrors();

	// virtual resolution scaling
	
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	logOpenGLErrors();

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
	logOpenGLErrors();

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	logOpenGLErrors();

	glfwSwapBuffers(window);
}

uint32_t rotateCCW(uint32_t tile) {
	return vflip(transpose(tile));
}

uint32_t rotateCW(uint32_t tile) {
	return transpose(vflip(tile));
}

uint32_t hflip(uint32_t tile) {
	return tile ^ ((tile & DFLIP) ? VFLIP : HFLIP);
}

uint32_t vflip(uint32_t tile) {
	return tile ^ ((tile & DFLIP)? HFLIP : VFLIP);
}

uint32_t transpose(uint32_t tile) {
	return tile ^ DFLIP;
}

uint32_t dither(uint32_t tile, uint32_t x_mult, uint32_t y_mult, uint32_t mod, bool parity) {
	assert(x_mult < 8 && y_mult < 8 && mod <= 32 && mod > 0);
	return (x_mult << 29) | (y_mult << 26) | ((mod - 1) << 21) | (parity * 0x00100000u) | (tile & 0x000FFFFF);
}

uint32_t filter(uint32_t cset, float r, float g, float b) {
	uint32_t red = (uint32_t) ((1.f - clamp(r)) * 63) << 26;
	uint32_t green = (uint32_t) ((1.f - clamp(g)) * 63) << 20;
	uint32_t blue = (uint32_t) ((1.f - clamp(b)) * 63) << 14;
	return red | green | blue | (cset & 0x00003FFF);
}

