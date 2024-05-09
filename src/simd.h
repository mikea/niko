#pragma once

// avx2
#define SIMD_REG_SIZE (512 / 8)

#define SIMD_ALIGNED ALIGNED(SIMD_REG_SIZE)