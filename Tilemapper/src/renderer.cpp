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

const unsigned int simple_tilemap[] = {
	2,       0, 2|HFLIP,       0, 2|DFLIP,       0, 2|HFLIP|DFLIP, 0,
	2|VFLIP, 0, 2|HFLIP|VFLIP, 0, 2|DFLIP|VFLIP, 0, 2|HFLIP|DFLIP|VFLIP, 0,
	2, 0, rotateCW(2), 0, rotateCW(rotateCW(2)), 0, rotateCCW(2), 0,
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

	glGenBuffers(1, &tilemap_vbo); // TODO: separate into Chunk class/struct
	glBindBuffer(GL_ARRAY_BUFFER, tilemap_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(simple_tilemap), simple_tilemap, GL_DYNAMIC_DRAW);
	logOpenGLErrors();

	tileset_slot = tile_shader.getSlot("tileset");
	palette_slot = tile_shader.getSlot("palette");
	chunk_size_slot = tile_shader.getSlot("chunk_size");
	tile_size_slot = tile_shader.getSlot("tile_size");

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

	camera = glm::ortho(0.f, (float) width, 0.f, (float) height);
}

void Renderer::draw_frame() {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, v_width, v_height);
	glClear(GL_COLOR_BUFFER_BIT);
	logOpenGLErrors();

	// Draw tilemaps

	tile_shader.use();
	tile_shader.set(chunk_size_slot, 4);
	tile_shader.set(tileset_slot, tileset->bind(0));
	tile_shader.set(palette_slot, palette->bind(1));
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
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 16);
	logOpenGLErrors();

	// virtual resolution scaling
	
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	//glClear(GL_COLOR_BUFFER_BIT);
	logOpenGLErrors();

	scale_shader.use();
	scale_shader.set(virtual_screen_slot, framebuffer);
	scale_shader.set(sharpness_slot, max(scaling_sharpness, 0.f));

	glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glEnableVertexAttribArray(0);
	logOpenGLErrors();

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	logOpenGLErrors();

	glfwSwapBuffers(window);
}

unsigned int rotateCW(unsigned int tile) {
	return vflip(transpose(tile));
}

unsigned int rotateCCW(unsigned int tile) {
	return transpose(vflip(tile));
}

unsigned int hflip(unsigned int tile) {
	return tile ^ ((tile & DFLIP) ? VFLIP : HFLIP);
}

unsigned int vflip(unsigned int tile) {
	return tile ^ ((tile & DFLIP)? HFLIP : VFLIP);
}

unsigned int transpose(unsigned int tile) {
	return tile ^ DFLIP;
}