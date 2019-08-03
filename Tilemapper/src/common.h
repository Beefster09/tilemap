#pragma once

#include <cstdint>
#include <cmath>

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
typedef u32 HexColor;

enum StatusCode {
	OK = 0,
	INVALID_CHARS,
	INVALID_LEN,
	INVALID_FORMAT,
};

StatusCode parse_hex_color(const char* str, const char** end, HexColor* color);
StatusCode parse_bool(const char* str, const char** end, bool* out);

char* readFile(const char* filename);

constexpr float PI = 3.1415926535f;
constexpr float TAU = (float)(3.141592653589793238 * 2.0);

int _logOpenGLErrors(const char* caller, const char* file, int lineNo);

#ifdef NDEBUG
#define logOpenGLErrors() do{}while(0)
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

#ifdef _MSC_VER
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif
#define __FILE_BASENAME__ (strrchr(__FILE__, PATH_SEP) + 1)
#ifdef NDEBUG
#define ERR_LOG(FMT, ...) do{}while(0)
#define DBG_LOG(FMT, ...) do{}while(0)
#else
#define ERR_LOG(FMT, ...) fprintf(stderr, "[%s (line %03d in %s)] " FMT "\n", __func__, __LINE__, __FILE_BASENAME__, __VA_ARGS__)
#define DBG_LOG(FMT, ...) printf("[%s (line %03d in %s)] " FMT "\n", __func__, __LINE__, __FILE_BASENAME__, __VA_ARGS__)
#endif

/// Allocate some bytes from temp storage
void* _temp_alloc(size_t n_bytes);

/// Allocate and zero from temp storage
void* _temp_alloc0(size_t n_bytes);

/// This should be run at the end of every frame in each thread that uses temp storage
void temp_storage_clear();

#define temp_alloc(TYPE, N) ((TYPE*) _temp_alloc(sizeof(TYPE) * N))
#define temp_alloc0(TYPE, N) ((TYPE*) _temp_alloc0(sizeof(TYPE) * N))