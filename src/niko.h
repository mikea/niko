#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "status.h"
#include "str.h"

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
    TOK_STR,
  } tok;
  str_t text;
  union {
    int64_t i;
    double d;
    str_t s;
  } val;
} token_t;

token_t next_token(const char** s);

// type
typedef enum { T_C8, T_I64, T_F64, T_ARR, T_FFI } type_t;
#define T_MAX (T_FFI + 1)

typedef char t_c8;
#define t_c8_enum T_C8

typedef int64_t t_i64;
#define t_i64_enum T_I64

typedef double t_f64;
#define t_f64_enum T_F64

struct array_t;
typedef struct array_t* t_arr;
#define t_arr_enum T_ARR

struct interpreter_t;
struct stack_t;
typedef STATUS_T (*t_ffi)(struct interpreter_t* inter, struct stack_t* s);
#define t_ffi_enum T_FFI

#define TYPE_ENUM(t) t##_enum

#define TYPE_FOREACH(f) f(t_c8) f(t_i64) f(t_f64) f(t_arr) f(t_ffi)

#define TYPE_ROW(v_c8, v_i64, v_f64, v_arr, v_ffi) \
  { v_c8, v_i64, v_f64, v_arr, v_ffi }

#define TYPE_ROW_FOREACH(f) TYPE_ROW(f(t_c8), f(t_i64), f(t_f64), f(t_arr), f(t_ffi))

#define TYPE_ROW_ID TYPE_ROW(T_C8, T_I64, T_F64, T_ARR, T_FFI)

static size_t type_sizeof_table[T_MAX] = TYPE_ROW_FOREACH(sizeof);
INLINE size_t type_sizeof(type_t t, size_t n) { return n * type_sizeof_table[t]; }

static const char* type_name_table[T_MAX] = TYPE_ROW("c8", "i64", "f64", "arr", "ffi");
INLINE const char* type_name(type_t t) { return type_name_table[t]; }

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
  type_t t;     // type
  size_t rc;    // ref count
  size_t n;     // number of elements
  size_t r;     // rank
  size_t d[0];  // dims
} array_t;

INLINE size_t array_sizeof(type_t t, size_t n, size_t r) {
  return sizeof(array_t) + dims_sizeof(r) + type_sizeof(t, n);
}
INLINE const size_t* array_dims(const array_t* a) { return a->d; }
INLINE shape_t array_shape(const array_t* a) { return (shape_t){a->r, array_dims(a)}; }

INLINE const void* array_data(const array_t* arr) { return (void*)(arr + 1) + dims_sizeof(arr->r); }
INLINE size_t array_data_sizeof(const array_t* a) { return type_sizeof(a->t, a->n); }
INLINE const void* array_data_i(const array_t* a, size_t i) { return array_data(a) + type_sizeof(a->t, i); }

INLINE array_t* array_assert_mut(array_t* arr) {
  assert(arr->rc == 1);
  return arr;
}
INLINE void* array_mut_data(array_t* arr) { return (void*)array_data(array_assert_mut(arr)); }
INLINE i64* restrict array_mut_data_i64(array_t* arr) { return (i64*)array_mut_data(arr); }

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

INLINE array_t* array_alloc_as(const array_t* a) { return array_alloc(a->t, a->n, array_shape(a)); }
INLINE array_t* array_new(type_t t, size_t n, shape_t s, const void* x) {
  array_t* a = array_alloc(t, n, s);
  memcpy(array_mut_data(a), x, type_sizeof(t, n));
  return a;
}
INLINE array_t* array_new_1d(type_t t, size_t n, const void* x) { return array_new(t, n, shape_1d(&n), x); }

INLINE bool is_atom(const array_t* a) { return a->r == 0; }
INLINE array_t* array_new_atom(type_t t, const void* x) { return array_new(t, 1, shape_atom(), x); }

#define __DEF_TYPE_HELPER(t) \
  INLINE array_t* atom_##t(t v) { return array_new_atom(TYPE_ENUM(t), &v); }
TYPE_FOREACH(__DEF_TYPE_HELPER)

#define __DO_ARRAY_IMPL(a, t, i, p, u)                          \
  for (bool u##b = 1; u##b; u##b = 0)                           \
    for (t* restrict p = (t*)array_mut_data(a); u##b; u##b = 0) \
      for (size_t i = 0, u##n = a->n; i < u##n && u##b; i++, p++)

#define _DO_ARRAY_IMPL(a, t, i, p, u) __DO_ARRAY_IMPL(a, t, i, p, u)
#define DO_ARRAY(a, t, i, p) _DO_ARRAY_IMPL(a, t, i, p, UNIQUE(__))

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
INLINE result_t result_err(string_t msg) { return (result_t){.ok = false, .either.e = msg}; }
INLINE PRINTF(1, 2) result_t result_errf(const char* format, ...) {
  return result_err(VA_ARGS_FWD(format, string_vnewf(format, args)));
}
#define R_UNWRAP(r)                                         \
  ({                                                        \
    result_t __result = (r);                                \
    if (!__result.ok) return status_err(__result.either.e); \
    __result.either.a;                                     \
  })

// stack

typedef struct stack_t stack_t;
struct stack_t {
  array_t** bottom;
  size_t l;
  size_t size;
};

INLINE stack_t* stack_new() { return calloc(1, sizeof(stack_t)); }
INLINE void stack_clear(stack_t* s) {
  DO(i, s->l) array_dec_ref(s->bottom[i]);
  s->l = 0;
}
INLINE void stack_free(stack_t* s) {
  stack_clear(s);
  free(s->bottom);
  free(s);
}
INLINE size_t stack_len(const stack_t* s) { return s->l; }
INLINE bool stack_is_empty(const stack_t* s) { return !stack_len(s); }
INLINE void stack_grow(stack_t* s) {
  s->size = (s->size + 1) * 2;
  s->bottom = reallocarray(s->bottom, sizeof(array_t*), s->size);
}
INLINE void stack_push(stack_t* stack, const array_t* a) {
  if (stack->l == stack->size) stack_grow(stack);
  stack->bottom[stack->l++] = (array_t*)a;
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  assert(stack->l > 0);
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->bottom[--stack->l]; }
INLINE void stack_drop(stack_t* s) { array_dec_ref(stack_pop(s)); }
INLINE array_t* stack_i(stack_t* s, size_t i) {
  assert(i < s->l);
  return s->bottom[s->l - i - 1];
}
INLINE array_t* stack_peek(stack_t* s) { return stack_i(s, 0); }
INLINE void stack_print(stack_t* stack) {
  DO(i, stack->l) {
    if (i > 0) printf(" ");
    printf("%pA", stack->bottom[i]);
  }
}

// dictionary

typedef struct {
  string_t k;
  array_t* v;
} dict_entry_t;

GEN_VECTOR(entry_vector, dict_entry_t);

extern entry_vector_t global_dict;

// interpreter
struct interpreter_t {
  entry_vector_t dict;
  stack_t* stack;
  size_t arr_level;
  size_t arr_marks[256];
  FILE* out;
  const char* line;
};
typedef struct interpreter_t interpreter_t;
RESULT_T interpreter_read_next_word(interpreter_t* inter);
STATUS_T interpreter_word(interpreter_t* inter, array_t* a);

// words

#define DEF_WORD_HANDLER(name) STATUS_T name(interpreter_t* inter, stack_t* stack)

#define DEF_WORD_HANDLER_1_1(n)                                           \
  INLINE RESULT_T n##_impl(const array_t* x);                             \
  DEF_WORD_HANDLER(n) {                                                   \
    R_CHECK(!stack_is_empty(stack), "stack underflow: 1 value expected"); \
    own(array_t) x = stack_pop(stack);                                    \
    result_t result = n##_impl(x);                                        \
    if (result.ok) {                                                      \
      stack_push(stack, result.either.a);                                 \
      return status_ok();                                                 \
    } else {                                                              \
      return status_err(result.either.e);                                 \
    }                                                                     \
  }                                                                       \
  INLINE RESULT_T n##_impl(const array_t* x)

#define REGISTER_WORD(w, n)                                                            \
  STATUS_T w_##n(interpreter_t* inter, stack_t* stack);                                \
  CONSTRUCTOR void w_##n##_register() {                                                \
    entry_vector_add(&global_dict, (dict_entry_t){string_newf(w), atom_t_ffi(w_##n)}); \
  }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)
