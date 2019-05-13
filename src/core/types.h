#pragma once

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;

// SOA Containers for vector types, not used in practice so far...
struct Float2Stream {
	float* x = nullptr;
	float* y = nullptr;
	int64  count = 0;
};

struct Float3Stream {
	float* x = nullptr;
	float* y = nullptr;
	float* z = nullptr;
	int64  count = 0;
};

struct Float4Stream {
	float* x = nullptr;
	float* y = nullptr;
	float* z = nullptr;
	float* w = nullptr;
	int64  count = 0;
};

template <typename T>
struct Range {
    union {
        T beg = 0, x, min;
    };
    union {
        T end = 0, y, max;
    };

    constexpr Range() = default;
    constexpr Range(T lo, T hi) : beg(lo), end(hi){};

    constexpr operator bool() const { return beg != end && beg < end; }
	constexpr Range& operator +=(T val) {
		beg += val;
		end += val;
		return *this;
	}
	constexpr Range& operator -=(T val) {
		beg -= val;
		end -= val;
		return *this;
	}
    constexpr int64 size() const { return end - beg; }
};

template <typename T>
constexpr bool operator==(const Range<T>& r_a, const Range<T>& r_b) {
    return r_a.beg == r_b.beg && r_a.end == r_b.end;
}

template <typename T>
constexpr bool operator!=(const Range<T>& r_a, const Range<T>& r_b) {
    return !(r_a == r_b);
}

template <typename T>
constexpr Range<T> operator+(const Range<T>& range, T val) {
	return { range.x + val, range.y + val };
}

template <typename T>
constexpr Range<T> operator-(const Range<T>& range, T val) {
	return { range.x - val, range.y - val };
}