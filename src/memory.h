#pragma once

#define ALIGNED(n) ATTR(aligned(n))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)           __ALIGN_MASK(x, (typeof(x))(a)-1)

#define CONSTRUCTOR ATTR(constructor)

#define DEF_CLEANUP(t, free_fn)                               \
  typedef t*  own_##t;                                        \
  INLINE void t##_free(void* p) {                             \
    if (p) free_fn((t*)p);                                    \
  }                                                           \
  INLINE void t##_unwind(void* ctx, void* p) { t##_free(p); } \
  INLINE void t##_cleanup(t** p) {                            \
    t##_free(*p);                                             \
    *p = NULL;                                                \
  }                                                           \
  INLINE void t##_cleanup_protected(t** p) {                  \
    t##_free(*p);                                             \
    *p = NULL;                                                \
  }

DEF_CLEANUP(char, free)
DEF_CLEANUP(FILE, fclose)

#define own(t)    CLEANUP(t##_cleanup) own_##t
#define borrow(t) t*

#define protected(t)  CLEANUP(t##_cleanup_protected) t*
#define protect(expr) (expr)
