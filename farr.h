#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define DO(v, l) for (size_t v = 0; v < l; v++)
#define DBG(...)                                 \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__);                  \
  fprintf(stderr, "\n");

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

// unowned string
typedef struct {
  size_t l;
  const char* p;
} str_t;

INLINE str_t str_new(const char* b, const char* e) { return (str_t){e - b, b}; }
INLINE str_t str_fromc(const char* c) { return (str_t){strlen(c), c}; }
INLINE size_t str_len(const str_t s) { return s.l; }
INLINE bool str_eq(const str_t s1, const str_t s2) { return s1.l == s2.l && !memcmp(s1.p, s2.p, s1.l); }
INLINE bool str_eqc(const str_t s, const char* c) { return str_eq(s, str_fromc(c)); }
INLINE void str_fprint(const str_t s, FILE* f) { DO(i, str_len(s)) putc(*(s.p + i), f); }
INLINE void str_print(const str_t s) { str_fprint(s, stdout); }
INLINE char str_i(const str_t s, size_t i) { return *(s.p + i); }
INLINE char* str_toc(str_t s) {
  char* c = malloc(s.l + 1);
  memcpy(c, s.p, s.l);
  c[s.l] = 0;
  return c;
}

// owned string
typedef struct {
  size_t l;
  const char* p;
} string_t;

static_assert(sizeof(string_t) == sizeof(str_t), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, l) == offsetof(str_t, l), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, p) == offsetof(str_t, p), "string_t & str_t binary layout should match");

INLINE void string_free(string_t s) { free((void*)s.p); }
INLINE string_t string_vnewf(const char* format, va_list args) {
  char* p;
  size_t l = vasprintf(&p, format, args);
  return (string_t){l, p};
}
INLINE PRINTF(1, 2) string_t string_newf(const char* format, ...) {
  return VA_ARGS_FWD(format, string_vnewf(format, args));
}
INLINE str_t string_as_str(string_t s) { return (str_t){s.l, s.p}; }

// status
typedef struct {
  bool ok;
  union {
    string_t s;
  } u;
} status_t;
#define STATUS_T WARN_UNUSED status_t
INLINE status_t status_ok() { return (status_t){true}; }
INLINE status_t status_err(string_t msg) { return (status_t){.ok = false, .u.s = msg}; }
INLINE PRINTF(1, 2) status_t status_errf(const char* format, ...) {
  return status_err(VA_ARGS_FWD(format, string_vnewf(format, args)));
}
INLINE bool status_is_ok(status_t s) { return s.ok; }
INLINE bool status_is_err(status_t s) { return !s.ok; }
INLINE status_t status_assert_err(status_t s) {
  assert(!s.ok);
  return s;
}
INLINE str_t status_msg(status_t s) { return string_as_str(status_assert_err(s).u.s); }
INLINE void status_print_error(status_t s) {
  str_t msg = status_msg(s);
  fprintf(stderr, "ERROR: %pS\n", &msg);
}

#define R_ERR(expr)                                   \
  do {                                                \
    status_t __status__ = (expr);                     \
    if (status_is_err(__status__)) return __status__; \
  } while (0)

// token
typedef struct {
  enum {
    TOK_EOF,
    TOK_ERR,
    TOK_I64,
    TOK_F64,
    TOK_WORD,
    TOK_ARR_OPEN,
    TOK_ARR_CLOSE,
  } tok;
  str_t text;
} token_t;

token_t next_token(const char** s);

// type
typedef enum { T_I64, T_F64 } type_t;
#define T_MAX (T_F64 + 1)
#define TYPE_ROW(v_i64, v_f64) \
  { v_i64, v_f64 }
static size_t type_sizeof_table[T_MAX] = TYPE_ROW(sizeof(i64), sizeof(f64));
INLINE size_t type_sizeof(type_t t, size_t n) { return n * type_sizeof_table[t]; }

// shape
typedef struct {
  size_t r;  // rank
  const size_t* d;
} shape_t;

INLINE size_t dims_sizeof(size_t r) { return r * sizeof(size_t); }
INLINE shape_t* shape_new(size_t r) {
  shape_t* s = malloc(sizeof(shape_t) + dims_sizeof(r));
  s->r = r;
  s->d = (size_t*)(s + 1);
  return s;
}
INLINE void shape_free(shape_t* s) { free(s); }
DEF_CLEANUP(shape_t, shape_free);
INLINE shape_t shape_atom() { return (shape_t){0, NULL}; }
INLINE shape_t shape_1d(const size_t* d) { return (shape_t){1, d}; }
INLINE shape_t shape_create(size_t r, const size_t* d) { return (shape_t){r, d}; }
INLINE bool shape_eq(shape_t s1, shape_t s2) { return s1.r == s2.r && !memcmp(s1.d, s2.d, dims_sizeof(s1.r)); }
INLINE size_t shape_len(shape_t s) {
  size_t y = 1;
  DO(i, s.r) y *= s.d[i];
  return y;
}
INLINE shape_t* shape_extend(shape_t s, size_t sz) {
  shape_t* r = shape_new(s.r + 1);
  size_t* d = (size_t*)r->d;
  memcpy(d + 1, s.d, dims_sizeof(s.r));
  *d = sz;
  return r;
}

// array: (header, dims, data)
typedef struct {
  type_t t;   // type
  size_t rc;  // ref count
  size_t n;   // number of elements
  size_t r;   // rank
} array_t;

INLINE size_t array_sizeof(type_t t, size_t n, size_t r) {
  return sizeof(array_t) + dims_sizeof(r) + type_sizeof(t, n);
}
INLINE const size_t* array_dims(const array_t* a) { return (size_t*)(a + 1); }
INLINE shape_t array_shape(const array_t* a) { return (shape_t){a->r, array_dims(a)}; }

INLINE const void* array_data(const array_t* arr) { return (void*)(arr + 1) + dims_sizeof(arr->r); }
INLINE size_t array_data_sizeof(const array_t* a) { return type_sizeof(a->t, a->n); }
INLINE const i64* array_data_i64(const array_t* arr) { return (i64*)array_data(arr); }

INLINE array_t* array_assert_mut(array_t* arr) {
  assert(arr->rc == 1);
  return arr;
}
INLINE void* array_mut_data(array_t* arr) { return (void*)array_data(array_assert_mut(arr)); }
INLINE i64* array_mut_data_i64(array_t* arr) { return (i64*)array_mut_data(arr); }

INLINE array_t* array_alloc(type_t t, size_t n, shape_t s) {
  assert(shape_len(s) == n);
  array_t* a = malloc(array_sizeof(t, n, s.r));
  a->t = t;
  a->rc = 1;
  a->n = n;
  a->r = s.r;
  memcpy(a + 1, s.d, dims_sizeof(s.r));
  return a;
}
INLINE void array_free(array_t* arr) { free(arr); }
INLINE array_t* array_inc_ref(array_t* arr) {
  arr->rc++;
  return arr;
}
INLINE void array_dec_ref(array_t* arr) {
  if (!--arr->rc) array_free(arr);
}
DEF_CLEANUP(array_t, array_dec_ref);

INLINE array_t* array_new(type_t t, size_t n, shape_t s, const void* x) {
  array_t* a = array_alloc(t, n, s);
  memcpy(array_mut_data(a), x, type_sizeof(t, n));
  return a;
}
INLINE bool is_atom(const array_t* a) { return a->r == 0 && a->n == 1; }
INLINE array_t* atom_i64(i64 i) { return array_new(T_I64, 1, shape_atom(), &i); }
INLINE array_t* atom_f64(f64 f) { return array_new(T_F64, 1, shape_atom(), &f); }

// result
typedef struct {
  bool ok;
  union {
    array_t* a;
    string_t e;
  } either;
} result_t;
#define RESULT_T WARN_UNUSED result_t
INLINE result_t result_ok(array_t* a) { return (result_t){.ok = true, .either.a = a}; }

// stack

typedef struct {
  array_t** bottom;
  size_t l;
  size_t size;
} stack_t;

INLINE stack_t* stack_new() { return calloc(1, sizeof(stack_t)); }
INLINE void stack_free(stack_t* s) {
  DO(i, s->l) array_dec_ref(s->bottom[i]);
  free(s->bottom);
  free(s);
}
INLINE size_t stack_len(const stack_t* s) { return s->l; }
INLINE bool stack_is_empty(const stack_t* s) { return !stack_len(s); }
INLINE void stack_grow(stack_t* s) {
  s->size = (s->size + 1) * 2;
  s->bottom = reallocarray(s->bottom, sizeof(array_t*), s->size);
}
INLINE void stack_push(stack_t* stack, array_t* a) {
  if (stack->l == stack->size) stack_grow(stack);
  stack->bottom[stack->l++] = a;
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  assert(stack->l > 0);
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->bottom[--stack->l]; }
INLINE array_t* stack_peek(stack_t* s) { return stack_assert_not_empty(s)->bottom[s->l - 1]; }
INLINE void stack_drop(stack_t* s) { array_dec_ref(stack_pop(s)); }
INLINE array_t* stack_i(stack_t* s, size_t i) {
  assert(i < s->l);
  return s->bottom[s->l - i - 1];
}
INLINE void stack_print(stack_t* stack) {
  DO(i, stack->l) {
    if (i > 0) printf(" ");
    printf("%pA", stack->bottom[i]);
  }
}
