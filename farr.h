#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int64_t i64;
typedef double f64;
static_assert(sizeof(f64) == sizeof(i64));

#define INLINE static inline
#define WARN_UNUSED __attribute__((warn_unused_result))
#define UNREACHABLE __builtin_unreachable()
#define DO(v, l) for (size_t v = 0; v < l; v++)

#define defer defer__2(__COUNTER__)
#define defer__2(X) defer__3(X)
#define defer__3(X) defer__4(defer__id##X)
#define defer__4(ID)                                  \
  auto void ID##func(char(*)[]);                      \
  __attribute__((cleanup(ID##func))) char ID##var[0]; \
  void ID##func(char(*ID##param)[])

// owned string
typedef struct {
  const char* b;
  size_t s;
} string_t;

// unowned string
typedef struct {
  const char* b;
  size_t s;
} str_t;

INLINE str_t str_new(const char* b, const char* e) { return (str_t){b, e - b}; }
INLINE str_t str_fromc(const char* c) { return (str_t){c, strlen(c)}; }
INLINE size_t str_len(const str_t s) { return s.s; }
INLINE bool str_eq(const str_t s1, const str_t s2) { return s1.s == s2.s && !memcmp(s1.b, s2.b, s1.s); }
INLINE bool str_eqc(const str_t s, const char* c) { return str_eq(s, str_fromc(c)); }
INLINE void str_fprint(const str_t s, FILE* f) { DO(i, str_len(s)) putc(*(s.b + i), f); }
INLINE void str_print(const str_t s) { str_fprint(s, stdout); }

// status
typedef struct {
  string_t* s;
} status_t;
INLINE status_t status_ok() { return (status_t){NULL}; }
INLINE status_t status_err(string_t* msg) { return (status_t){msg}; }

// token
typedef struct {
  enum { TOK_EOF, TOK_ERR, TOK_INT64, TOK_WORD } tok;
  str_t text;
} token_t;

token_t next_token(const char** s);

// type
typedef enum { T_I64, T_F64 } type_t;

INLINE size_t type_sizeof(type_t t, size_t n) {
  switch (t) {
    case T_I64: return n * sizeof(i64);
    case T_F64: return n * sizeof(f64);
  }
  __builtin_unreachable();
}

// shape
typedef struct {
  size_t r;         // rank
  const size_t* d;  // dims
} shape_t;
INLINE shape_t shape_atom() { return (shape_t){0, NULL}; }

INLINE size_t shape_size(shape_t s) {
  size_t y = 1;
  DO(i, s.r) y *= s.d[i];
  return y;
}

// array: (header, dims, data)
typedef struct {
  type_t t;   // type
  size_t rc;  // ref count
  size_t n;   // number of elements
  size_t r;   // rank
} array_t;

INLINE size_t shape_sizeof(size_t r) { return r * sizeof(size_t); }
INLINE size_t array_sizeof(type_t t, size_t n, size_t r) {
  return sizeof(array_t) + shape_sizeof(r) + type_sizeof(t, n);
}
INLINE const shape_t array_shape(const array_t* a) { return (shape_t){a->r, (size_t*)(a + 1)}; }
INLINE const void* array_data(const array_t* arr) { return (void*)(arr + 1) + shape_sizeof(arr->r); }

INLINE array_t* array_assert_mut(array_t* arr) {
  assert(arr->rc == 1);
  return arr;
}
INLINE void* array_mut_data(array_t* arr) { return (void*)array_data(array_assert_mut(arr)); }
INLINE size_t* array_mut_shape(array_t* a) { return (size_t*)(array_assert_mut(a) + 1); }

INLINE array_t* array_alloc(type_t t, size_t n, shape_t s) {
  assert(shape_size(s) == n);
  array_t* a = malloc(array_sizeof(t, n, s.r));
  a->t = t;
  a->rc = 1;
  a->n = n;
  a->r = s.r;
  if (s.r > 0) memcpy(array_mut_shape(a), s.d, shape_sizeof(s.r));
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
INLINE array_t* array_new(type_t t, size_t n, shape_t s, const void* x) {
  array_t* a = array_alloc(t, n, s);
  memcpy(array_mut_data(a), x, type_sizeof(t, n));
  return a;
}
INLINE array_t* atom_i64(i64 i) { return array_new(T_I64, 1, shape_atom(), &i); }

// result
typedef struct {
  bool ok;
  union {
    array_t* a;
    string_t* e;
  } either;
} result_t;
INLINE result_t result_ok(array_t* a) { return (result_t){.ok = true, .either.a = a}; }

// forward declarations
void register_printf_extensions();