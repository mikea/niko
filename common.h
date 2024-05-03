#pragma once

#include <stdint.h>

typedef int64_t i64;
typedef double f64;
static_assert(sizeof(f64) == sizeof(i64));

#define INLINE static inline
#define ATTR(attr) __attribute__((attr))
#define CLEANUP(func) ATTR(cleanup(func))
#define WARN_UNUSED ATTR(warn_unused_result)
#define CONSTRUCTOR ATTR(constructor)
#define PRINTF(i, j) ATTR(format(printf, i, j))

#define UNREACHABLE __builtin_unreachable()
#define NOT_IMPLEMENTED \
  assert(false);        \
  abort();
#define DO(var, l) for (size_t var = 0; var < l; var++)
#define DBG(...)                                   \
  do {                                             \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                  \
    fprintf(stderr, "\n");                         \
  } while (0)

#define VA_ARGS_FWD(last, call)   \
  ({                              \
    va_list args;                 \
    va_start(args, last);         \
    typeof(call) __result = call; \
    va_end(args);                 \
    __result;                     \
  })

// memory management

#define DEF_CLEANUP(t, free_fn)    \
  INLINE void t##_cleanup(t** p) { \
    free_fn(*p);                   \
    *p = NULL;                     \
  }

DEF_CLEANUP(char, free);
DEF_CLEANUP(FILE, fclose);

#define own(t) CLEANUP(t##_cleanup) t*

#define GEN_VECTOR(name, t)                        \
  typedef struct {                                 \
    size_t l;                                      \
    size_t s;                                      \
    t* d;                                          \
  } name##_t;                                      \
  INLINE void name##_grow(name##_t* v) {           \
    v->s = (v->s + 1) * 2;                         \
    v->d = reallocarray(v->d, sizeof(t), v->s);    \
  }                                                \
  INLINE void name##_grow_if_needed(name##_t* v) { \
    if (v->l == v->s) name##_grow(v);              \
  }                                                \
  INLINE void name##_add(name##_t* v, t e) {       \
    name##_grow_if_needed(v);                      \
    v->d[v->l++] = e;                              \
  }
