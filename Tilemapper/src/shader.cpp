#include <glad/glad.h>
#include <glfw3.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

#include "common.h"
#include "shader.h"

constexpr int INFO_LOG_SIZE = 4096;

GLuint compileShader(const char* vertexShaderSource, const char* fragmentShaderSource) {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	int  success;
	char infoLog[INFO_LOG_SIZE];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, INFO_LOG_SIZE, NULL, infoLog);
		printf("Vertex Shader Compilation Failed!\n%s\n", infoLog);
		glDeleteShader(vertexShader);
		return -1;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, INFO_LOG_SIZE, NULL, infoLog);
		printf("Fragment Shader Compilation Failed!\n%s\n", infoLog);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return -1;
	}

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, INFO_LOG_SIZE, NULL, infoLog);
		printf("Shader Linking Failed!\n%s\n", infoLog);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return -1;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint compileShaderFromFiles(const char* vertexShaderFile, const char* fragmentShaderFile) {
	const char* vShader = readFile(vertexShaderFile);
	if (!vShader) {
		printf("Could not read vertex shader '%s'\n", vertexShaderFile);
		return -1;
	}

	const char* fShader = readFile(fragmentShaderFile);
	if (!vShader) {
		printf("Could not read fragment shader '%s'\n", fragmentShaderFile);
		delete[] vShader;
		return -1;
	}

	GLuint shaderProgram = compileShader(vShader, fShader);

	delete[] vShader, fShader;

	return shaderProgram;
}

GLuint Shader::activeProgram = -1;

Shader::Shader(GLuint shaderID, const char* vert_src, const char* frag_src) {
	assert(shaderID != -1);
	shaderProgram = shaderID; //compileShaderFromFiles(vert, frag);
	transformSlot = getSlot("transform");
	cameraSlot = getSlot("camera");

#ifndef NDEBUG
	printf("Shader [Vertex] %s -> [Fragment] %s\n", vert_src, frag_src);
	printFullInterface();
	printf("\n");
#endif
}

void Shader::printFullInterface() {
	GLint numSlots;
	glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numSlots);
	for (int index = 0; index < numSlots; index++) {
		GLenum type;
		GLsizei len;
		GLint size;
		char name[32];
		glGetActiveUniform(shaderProgram, index, sizeof(name), &len, &size, &type, name);
		int slot = getSlot(name);
		if (slot >= 0) {
			printf("[Slot %d] %s %s;\n", slot, glslTypeName(type), name);
		}
	}
}

void Shader::use() const {
	if (activeProgram != shaderProgram) {
		activeProgram = shaderProgram;
		glUseProgram(shaderProgram);
	}
}

int Shader::getSlot(const char* name) const {
	int slot = glGetUniformLocation(shaderProgram, name);
	return slot;
}

void Shader::set(int slot, int i) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform1i(slot, i);
}

void Shader::setUint(int slot, unsigned int u) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform1ui(slot, u);
}

void Shader::set(int slot, float f) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform1f(slot, f);
}

void Shader::set(int slot, float x, float y) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform2f(slot, x, y);
}

void Shader::set(int slot, glm::vec2 vec) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform2f(slot, XY(vec));
}

void Shader::set(int slot, glm::vec3 vec) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform3f(slot, XYZ(vec));
}

void Shader::set(int slot, glm::vec4 vec) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform4f(slot, XYZ(vec), vec.w);
}

void Shader::set(int slot, const glm::mat4& mat) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniformMatrix4fv(slot, 1, GL_FALSE, &mat[0][0]);
}

void Shader::set(int slot, Texture* tex) const {
	if (slot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniform1i(slot, tex->bind());
}

void Shader::setCamera(const glm::mat4& camera) const {
	if (cameraSlot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniformMatrix4fv(cameraSlot, 1, GL_FALSE, glm::value_ptr(camera));
}

void Shader::setTransform(const glm::mat4& transform) const {
	if (transformSlot < 0) return;
	assert(activeProgram == shaderProgram);
	glUniformMatrix4fv(transformSlot, 1, GL_FALSE, glm::value_ptr(transform));
}

void Shader::setTransform(glm::vec2 position, glm::vec2 scale, float yawRot) const {
	if (transformSlot < 0) return;
	setTransform(
		glm::translate(glm::vec3{XY(position), 0.f})
		* glm::rotate(yawRot, glm::vec3{0.f, 0.f, 1.f})
		* glm::scale(glm::vec3{XY(scale), 1.f})
	);
}

Shader::~Shader() {
	if (shaderProgram != -1) {
		glDeleteProgram(shaderProgram);
	}
}

Shader& Shader::operator = (Shader&& otherShader) {
	if (shaderProgram != -1) {
		glDeleteProgram(shaderProgram);
	}
	shaderProgram = otherShader.shaderProgram;
	transformSlot = otherShader.transformSlot;
	cameraSlot    = otherShader.cameraSlot;

	otherShader.shaderProgram = -1;

	return *this;
}

Shader::Shader(Shader&& other) {
	*this = std::move(other);
}
