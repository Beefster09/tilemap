#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <glad/glad.h>
#include <glfw3.h>

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

const char* glslTypeName(GLenum type) {
	switch (type) {
	case GL_FLOAT: return "float";
	case GL_FLOAT_VEC2: return "vec2";
	case GL_FLOAT_VEC3: return "vec3";
	case GL_FLOAT_VEC4: return "vec4";
	case GL_INT: return "int";
	case GL_BOOL: return "bool";
	case GL_UNSIGNED_INT: return "uint";
	case GL_FLOAT_MAT3: return "mat3";
	case GL_FLOAT_MAT4: return "mat4";
	case GL_SAMPLER_2D: return "sampler2D";
	case GL_SAMPLER_2D_RECT: return "sampler2DRect";
	case GL_SAMPLER_2D_ARRAY: return "sampler2DArray";
	case GL_INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_RECT: return "usamplerRect";
		// There are more, but this is good enough for now.
	default: return "???";
	}
}

#define T(ENUM) case ENUM: return #ENUM
const char* glEnumName(GLenum type) {
	switch (type) {
		T(GL_FLOAT);
		T(GL_UNSIGNED_INT);
		T(GL_INT);
		T(GL_TEXTURE_2D);
		T(GL_TEXTURE_2D_ARRAY);
		T(GL_TEXTURE_RECTANGLE);
		// There are more, but this is good enough for now.
		default: return "???";
	}
}
#undef T

int _logOpenGLErrors(const char* caller, const char* file, int lineNo) {
	GLenum err;
	int err_count = 0;
	while ((err = glGetError()) != GL_NO_ERROR) {
		err_count++;
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
	return err_count;
}

static inline u32 double_nybbles(u32 x) {
	// expand ABCD into _A_B_C_D, shift & combine
	u32 r = x & 0x000F;
	r |= (x & 0x00F0) << 4;
	r |= (x & 0x0F00) << 8;
	r |= (x & 0xF000) << 12;
	return r | (r << 4);
}

HexColorParseStatusCode parse_hex_color(const char* str, const char** end, HexColor* color) {
	const char* local_end = nullptr;
	if (end == nullptr) {
		end = &local_end;
	}
	errno = 0;
	HexColor maybe_color = strtoul(str, const_cast<char**>(end), 16);
	if (errno == ERANGE) {
		return HEX_COLOR_INVALID_LEN;
	}
	auto count = (*end - str);
	for (int i = 0; i < count; i++) {
		if (!(str[i] >= '0' && str[i] <= '9'
			|| str[i] >= 'A' && str[i] <= 'F'
			|| str[i] >= 'a' && str[i] <= 'f')) {
			return HEX_COLOR_INVALID_CHARS;
		}
	}
	switch (count) {
	case 3:  // RGB
		*color = double_nybbles(maybe_color);
		*color <<= 8;
		*color |= 0xff;
		return HEX_COLOR_OK;
	case 6:  // RRGGBB
		*color = (maybe_color << 8) | 0xff;
		return HEX_COLOR_OK;

	case 4:  // RGBA
		*color = double_nybbles(maybe_color);
		return HEX_COLOR_OK;
	case 8:  // RRGGBBAA
		*color = maybe_color;
		return HEX_COLOR_OK;

	default:
		return HEX_COLOR_INVALID_LEN;
	}
}

// Temporary Storage - where things that live no longer than 1 frame can be allocated
// Good for strings and similar shit
// MAYBE? have this be opt-in per thread since the audio mixer thread (and possibly others) might not need it.

#ifndef TEMP_STORAGE_SIZE
#define TEMP_STORAGE_SIZE (16 * 1024) // 16KB
#endif

thread_local char* TEMP_STORAGE = nullptr;
thread_local char* temp_storage_current = nullptr;

void* _temp_alloc(size_t n_bytes) {
	if (TEMP_STORAGE == nullptr) {
		temp_storage_current = TEMP_STORAGE = (char*) malloc(TEMP_STORAGE_SIZE);
		assert(TEMP_STORAGE && "Unable to initialize temp storage.");
	}
	if (n_bytes == 0) return nullptr;
	if (temp_storage_current + n_bytes > TEMP_STORAGE + TEMP_STORAGE_SIZE) {
		return nullptr;
	}
	else {
		char* ret = temp_storage_current;
		temp_storage_current += n_bytes;
		// align to the nearest 8 bytes;
		temp_storage_current += 7;
		*(intptr_t*)&temp_storage_current &= ~7;
		return ret;
	}
}

void* _temp_alloc0(size_t n_bytes) {
	void* mem = _temp_alloc(n_bytes);
	memset(mem, 0, n_bytes);
	return mem;
}

void temp_storage_clear() {
	temp_storage_current = TEMP_STORAGE;
}