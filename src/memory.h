#pragma once

#include "panic.h"

#define ALIGNED(n) ATTR(aligned(n))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a)-1)

#define CONSTRUCTOR ATTR(constructor)

#define DEF_CLEANUP(t, free_fn)                \
  INLINE void t##_cleanup(t** p) {             \
    if (*p) free_fn(*p);                       \
    *p = NULL;                                 \
  }                                            \
  INLINE void t##_panic_handler(void* p) {     \
    if (p) free_fn((t*)p);                     \
  }                                            \
  INLINE void t##_cleanup_protected(t** p) {   \
    unwind_handler_pop(t##_panic_handler, *p); \
    if (*p) free_fn(*p);                       \
    *p = NULL;                                 \
  }

DEF_CLEANUP(char, free);
DEF_CLEANUP(FILE, fclose);

#define own(t) CLEANUP(t##_cleanup) t*
#define protected(t) CLEANUP(t##_cleanup_protected) t*
#define borrow(t) t*
