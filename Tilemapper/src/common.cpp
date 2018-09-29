#include <cstdio>
#include <cstdint>

#include <glad/glad.h>

#include "common.h"

char* readFile(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) return nullptr;

	fseek(file, 0, SEEK_END);

	size_t fsize = ftell(file);
	char* buffer = new char[fsize + 1];

	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, fsize, file) == fsize) {
		buffer[fsize] = 0;
		return buffer;
	}
	else {
		delete[] buffer;
		return nullptr;
	}
}

void _logOpenGLErrors(const char* caller, const char* file, int lineNo) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
		const char* message = "???";
		switch (err) {
		case GL_INVALID_ENUM:  message = "Invalid Enum"; break;
		case GL_INVALID_VALUE: message = "Invalid Value"; break;
		case GL_INVALID_OPERATION: message = "Invalid Operation"; break;
		case GL_OUT_OF_MEMORY: message = "Unable to allocate sufficient memory"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: message = "Invalid framebuffer operation"; break;

#ifdef GL_STACK_OVERFLOW
		case GL_STACK_OVERFLOW: message = "Stack Overflow"; break;
#endif
#ifdef GL_STACK_UNDERFLOW
		case GL_STACK_UNDERFLOW: message = "Stack Underflow"; break;
#endif
#ifdef GL_CONTEXT_LOST
		case GL_CONTEXT_LOST: message = "Context Lost"; break;
#endif
#ifdef GL_TABLE_TOO_LARGE
		case GL_TABLE_TOO_LARGE: message = "Table Too Large"; break;
#endif
		}
		printf("**OPENGL ERROR** (%s: in %s, line %d) %s\n", file, caller, lineNo, message);
	}
}

const char* glTypeName(GLenum type) {
	switch (type) {
	case GL_FLOAT: return "float";
	case GL_FLOAT_VEC2: return "vec2";
	case GL_FLOAT_VEC3: return "vec3";
	case GL_FLOAT_VEC4: return "vec4";
	case GL_INT: return "int";
	case GL_UNSIGNED_INT: return "uint";
	case GL_FLOAT_MAT3: return "mat3";
	case GL_FLOAT_MAT4: return "mat4";
	case GL_SAMPLER_2D: return "sampler2D";
	case GL_SAMPLER_2D_RECT: return "sampler2DRect";
	case GL_SAMPLER_2D_ARRAY: return "sampler2DArray";
	case GL_INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
		// There are more, but this is good enough for now.
	default: return "???";
	}
}