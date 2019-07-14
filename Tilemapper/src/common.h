#pragma once

#include <glfw3.h>

char* readFile(const char* filename);

constexpr float PI = 3.1415926535f;
constexpr float TAU = (float)(3.141592653589793238 * 2.0);

int _logOpenGLErrors(const char* caller, const char* file, int lineNo);

#ifdef NDEBUG
#define logOpenGLErrors()
#else
#define logOpenGLErrors() _logOpenGLErrors(__func__, __FILE__, __LINE__)
#endif

#define XY(VEC) VEC.x, VEC.y
#define XZ(VEC) VEC.x, VEC.z
#define XYZ(VEC) VEC.x, VEC.y, VEC.z

#define BIT(B) (1ULL << (B))
#define BITOF(X, B) ((X) & BIT(B))
#define BITSET(X, B) ((X) |= BIT(B))
#define BITCLEAR(X, B) ((X) &= ~BIT(B))

template<typename T>
inline T max(T a, T b) {
	return a > b? a : b;
}

template<typename T>
inline T min(T a, T b) {
	return a < b? a : b;
}

template<typename T>
inline T clamp(T a, T lo = 0, T hi = 1) {
	if (a < lo) return lo;
	if (a > hi) return hi;
	return a;
}

const char* glEnumName(GLenum type);
const char* glslTypeName(GLenum type);

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
