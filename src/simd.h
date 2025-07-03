#pragma once

#include <cstddef>
#include <cstdint>

#include "common.h"

// avx2
#define SIMD_REG_WIDTH_BITS  512
#define SIMD_REG_WIDTH_BYTES ((SIMD_REG_WIDTH_BITS) / 8)
#define SIMD_MAX_WIDTH(t)    ((SIMD_REG_WIDTH_BYTES) / sizeof(t))

#define SIMD_ALIGNMENT (SIMD_REG_WIDTH_BYTES)
#define SIMD_ALIGNED   ALIGNED(SIMD_ALIGNMENT)

#define SIMD_ALIGN_BYTES(x)   ALIGN(x, SIMD_ALIGNMENT)
#define SIMD_ALIGN_TYPE(t, x) ALIGN(x, SIMD_ALIGNMENT / sizeof(t))

#define DEF_MAX_VECTOR(t) typedef t vmax_##t __attribute__((vector_size(SIMD_REG_WIDTH_BYTES)))

DEF_MAX_VECTOR(char);
DEF_MAX_VECTOR(i64);
DEF_MAX_VECTOR(f64);