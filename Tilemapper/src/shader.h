#pragma once

#include <cassert>

#include <glm/glm.hpp>

#include "common.h"
#include "texture.h"

GLuint compileShader(const char* vertexShaderSource, const char* fragmentShaderSource);
GLuint compileShaderFromFiles(const char* vertexShaderFile, const char* fragmentShaderFile);

class Shader {
private:
	static GLuint activeProgram;

	GLuint shaderProgram = -1;
	int transformSlot;
	int cameraSlot;

public:
	//Shader(const char* vertexShaderFile, const char* fragmentShaderFile);
	Shader(GLuint shaderID, const char* vert_src, const char* frag_src);
	Shader(Shader&& other);
	Shader(const Shader& other) = delete;
	Shader() = default;
	~Shader();

	Shader& operator = (const Shader& other) = delete;
	Shader& operator = (Shader&& other);

	inline operator bool() const { return shaderProgram != -1; }

	void use() const;

	int getSlot(const char* name) const;

	void set(int slot, int i) const;
	void setUint(int slot, unsigned int u) const;
	void set(int slot, float f) const;
	void set(int slot, glm::vec2 vec) const;
	void set(int slot, glm::vec3 vec) const;
	void set(int slot, glm::vec4 vec) const;
	void set(int slot, const glm::mat4& mat) const;
	void set(int slot, Texture* tex) const;

	void setCamera(const glm::mat4& mat) const;
	void setTransform(const glm::mat4& mat) const;
	void setTransform(glm::vec2 position, glm::vec2 scale = {1.f, 1.f}, float rotation = 0.f) const;

	void printFullInterface();
};