#pragma once

#include <string.h>

#include "common.h"
#include "memory.h"
#include "simd.h"

// type
typedef enum { T_C8, T_I64, T_F64, T_ARR, T_FFI, T_DICT_ENTRY } type_t;
#define T_MAX (T_DICT_ENTRY + 1)

typedef char t_c8;
#define t_c8_enum T_C8

typedef int64_t t_i64;
#define t_i64_enum T_I64
#define t_i64_simd vmax_i64

typedef double t_f64;
#define t_f64_enum T_F64
#define t_f64_simd vmax_f64

struct array_t;
typedef struct array_t* t_arr;
#define t_arr_enum T_ARR

struct inter;
struct stack;
typedef void (*t_ffi)(struct inter* inter, struct stack* s);
#define t_ffi_enum T_FFI

typedef i64 t_dict_entry;
#define t_dict_entry_enum T_DICT_ENTRY

#define TYPE_ENUM(t) t##_enum
#define TYPE_SIMD(t) t##_simd

#define TYPE_FOREACH(f)      f(t_c8) f(t_i64) f(t_f64) f(t_arr) f(t_ffi) f(t_dict_entry)
#define TYPE_FOREACH_SIMD(f) f(t_i64, vmax_i64) f(t_f64, vmax_f64)

#define TYPE_ROW(v_c8, v_i64, v_f64, v_arr, v_ffi, v_dict_entry) \
  { v_c8, v_i64, v_f64, v_arr, v_ffi, v_dict_entry }

#define TYPE_ROW_FOREACH(f) TYPE_ROW(f(t_c8), f(t_i64), f(t_f64), f(t_arr), f(t_ffi), f(t_dict_entry))

#define TYPE_ROW_ID TYPE_ROW(T_C8, T_I64, T_F64, T_ARR, T_FFI)

static size_t type_sizeof_table[T_MAX] = TYPE_ROW_FOREACH(sizeof);
INLINE size_t type_sizeof(type_t t, size_t n) { return n * type_sizeof_table[t]; }

static const char* type_name_table[T_MAX] = TYPE_ROW("c8", "i64", "f64", "arr", "ffi", "dict");
INLINE const char* type_name(type_t t) { return type_name_table[t]; }

// dims

typedef size_t dim_t;

// shape
typedef struct {
  size_t       r;  // rank
  const dim_t* d;
} shape_t;

INLINE size_t   dims_sizeof(size_t r) { return r * sizeof(dim_t); }
INLINE shape_t* shape_new(size_t r) {
  shape_t* s = malloc(sizeof(shape_t) + dims_sizeof(r));
  s->r       = r;
  s->d       = (dim_t*)(s + 1);
  return s;
}
INLINE void shape_free(shape_t* s) { free(s); }
DEF_CLEANUP(shape_t, shape_free);

INLINE shape_t      shape_scalar() { return (shape_t){0, NULL}; }
INLINE shape_t      shape_1d(const dim_t* d) { return (shape_t){1, d}; }
INLINE shape_t      shape_create(size_t r, const dim_t* d) { return (shape_t){r, d}; }
INLINE bool         shape_eq(shape_t s1, shape_t s2) { return s1.r == s2.r && !memcmp(s1.d, s2.d, dims_sizeof(s1.r)); }
INLINE CONST size_t shape_len(shape_t s) {
  size_t y      = 1;
  DO(i, s.r) y *= s.d[i];
  return y;
}
INLINE shape_t* shape_extend(shape_t outer, shape_t inner) {
  shape_t* sh = shape_new(outer.r + inner.r);
  memcpy((dim_t*)sh->d, outer.d, dims_sizeof(outer.r));
  memcpy((dim_t*)sh->d + outer.r, inner.d, dims_sizeof(inner.r));
  return sh;
}
INLINE shape_t shape_suffix(shape_t s, size_t r) { return (shape_t){r, s.d + s.r - r}; }
INLINE shape_t shape_prefix(shape_t s, size_t r) { return (shape_t){r, s.d}; }

INLINE bool shape_is_cell(shape_t outer, shape_t inner) {
  return outer.r >= inner.r && !memcmp(outer.d + (outer.r - inner.r), inner.d, dims_sizeof(inner.r));
}
INLINE bool    shapes_are_compatible(shape_t s1, shape_t s2) { return shape_is_cell(s1, s2) || shape_is_cell(s2, s1); }
INLINE shape_t shape_max(shape_t s1, shape_t s2) {
  assert(shapes_are_compatible(s1, s2));
  return s1.r >= s2.r ? s1 : s2;
}

typedef enum { FLAG_QUOTE = 1 } flag_t;

// array: (header, dims, data)
struct array_t {
  type_t          t;   // type
  flag_t          f;   // flag
  size_t          rc;  // ref count
  size_t          n;   // number of elements
  struct array_t* owner;
  void* restrict p;  // simd aligned size and length if n is large enough
  size_t r;          // rank
  dim_t  d[0];       // dims
};
typedef struct array_t array_t;

array_t* array_alloc(type_t t, size_t n, shape_t s);
void     array_free(array_t* a);

INLINE array_t* array_alloc_shape(type_t t, shape_t s) { return array_alloc(t, shape_len(s), s); }
INLINE bool __array_data_simd_aligned(type_t t, size_t n) { return n >= 2 * SIMD_REG_WIDTH_BYTES / type_sizeof(t, 1); }
INLINE bool array_data_simd_aligned(const array_t* a) { return __array_data_simd_aligned(a->t, a->n); }

INLINE const dim_t* array_dims(const array_t* a) { return a->d; }
INLINE shape_t      array_shape(const array_t* a) { return (shape_t){a->r, array_dims(a)}; }

INLINE const array_t* __array_assert_type(const array_t* a, type_t t) {
  assert(a->t == t);
  return a;
}
INLINE array_t* __array_assert_mut(const array_t* arr) {
  assert(arr->rc <= 1 && !arr->owner);
  return (array_t*)arr;
}
INLINE array_t* __array_assert_simd_aligned(array_t* arr) {
  assert(array_data_simd_aligned(arr));
  return arr;
}

INLINE size_t      array_data_sizeof(const array_t* a) { return type_sizeof(a->t, a->n); }
INLINE const void* array_data(const array_t* arr) { return arr->p; }
INLINE void*       array_mut_data(array_t* arr) { return (void*)array_data(__array_assert_mut(arr)); }
INLINE const void* array_data_i(const array_t* a, size_t i) { return array_data(a) + type_sizeof(a->t, i); }
INLINE void* array_mut_data_i(const array_t* a, size_t i) { return (void*)array_data_i(__array_assert_mut(a), i); }

INLINE void array_dec_ref(array_t* arr);

INLINE array_t* array_inc_ref(array_t* arr) {
  arr->rc++;
  return arr;
}
INLINE void array_dec_ref(array_t* arr) {
  assert(arr->rc > 0);
  if (!--arr->rc) array_free(arr);
}
INLINE array_t* array_move(array_t* arr) {
  assert(arr->rc > 0);
  arr->rc--;
  return arr;
}
DEF_CLEANUP(array_t, array_dec_ref);

INLINE array_t* array_alloc_as(const array_t* a) { return array_alloc(a->t, a->n, array_shape(a)); }
INLINE array_t* array_new(type_t t, size_t n, shape_t s, const void* x) {
  array_t* a = array_alloc(t, n, s);
  memcpy(array_mut_data(a), x, type_sizeof(t, n));
  return a;
}
INLINE array_t* array_new_1d(type_t t, size_t n, const void* x) {
  dim_t d = n;
  return array_new(t, n, shape_1d(&d), x);
}
INLINE array_t* array_new_slice(array_t* x, size_t n, shape_t s, const void* p) {
  array_inc_ref(x);
  array_t* y = malloc(sizeof(array_t) + dims_sizeof(s.r));
  y->t       = x->t;
  y->rc      = 1;
  y->n       = n;
  y->r       = s.r;
  y->p       = (void*)p;
  y->owner   = x;
  memcpy(y + 1, s.d, dims_sizeof(s.r));
  return y;
}
INLINE array_t* array_new_copy(const array_t* a) { return array_new(a->t, a->n, array_shape(a), array_data(a)); }
INLINE array_t* array_cow(array_t* a) {
  if (a->rc == 1) return (array_t*)a;
  array_t* y = array_new_copy(a);
  array_dec_ref(a);
  return y;
}

INLINE bool     array_is_scalar(const array_t* a) { return a->r == 0; }
INLINE array_t* array_new_scalar(type_t t, const void* x) { return array_new(t, 1, shape_scalar(), x); }

#define __DEF_TYPE_HELPER(t)                                                                                  \
  INLINE array_t* array_new_scalar_##t(t v) { return array_new_scalar(TYPE_ENUM(t), &v); }                    \
  INLINE array_t* array_new_##t(size_t n, shape_t s, const t* x) { return array_new(TYPE_ENUM(t), n, s, x); } \
  INLINE const t* array_data_##t(const array_t* a) {                                                          \
    return (const t*)array_data(__array_assert_type(a, TYPE_ENUM(t)));                                        \
  }                                                                                                           \
  INLINE t* array_mut_data_##t(array_t* a) {                                                                  \
    return (t*)array_mut_data((array_t*)__array_assert_type(a, TYPE_ENUM(t)));                                \
  }

TYPE_FOREACH(__DEF_TYPE_HELPER)

#define __DEF_SIMD_HELPER(t, v)                                         \
  INLINE v* restrict array_mut_data_##v(array_t* a) {                   \
    return (v* restrict)array_mut_data(__array_assert_simd_aligned(a)); \
  }

TYPE_FOREACH_SIMD(__DEF_SIMD_HELPER)

#define __DO_ARRAY_IMPL(a, i, p, u, p_decl) \
  for (bool u##b = 1; u##b; u##b = 0)       \
    for (p_decl; u##b; u##b = 0)            \
      for (size_t i = 0, u##n = a->n; i < u##n && u##b; i++, p++)

#define _DO_ARRAY_IMPL(a, i, p, u, p_decl) __DO_ARRAY_IMPL(a, i, p, u, p_decl)
#define DO_MUT_ARRAY(a, t, i, p)           _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), t* restrict p = (t*)array_mut_data(a))
#define DO_ARRAY(a, t, i, p)               _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), const t* restrict p = (const t*)array_data(a))

INLINE void array_for_each_cell(array_t* x, size_t r, void (*callback)(size_t i, array_t* slice)) {
  CHECK(r <= x->r, "invalid rank: %ld > %ld", r, x->r);
  shape_t cell    = shape_suffix(array_shape(x), r);
  size_t  l       = shape_len(cell);
  size_t  stride  = type_sizeof(x->t, l);

  const void* ptr = array_data(x);
  DO(i, x->n / l) {
    own(array_t) y = array_new_slice(x, l, cell, ptr + stride * i);
    callback(i, y);
  }
}

INLINE array_t* array_get_cell(array_t* x, size_t r, const i64* c) {
  CHECK(r <= x->r, "invalid rank: %ld > %ld", r, x->r);

  shape_t a   = array_shape(x);
  size_t  off = 0;
  size_t  mul = 1;

  DOR(i, x->r) {
    if (i < r) off += mul * WRAP(c[i], a.d[i]);
    mul *= a.d[i];
  }

  shape_t cell = shape_suffix(array_shape(x), x->r - r);
  return array_new_slice(x, shape_len(cell), cell, array_data(x) + type_sizeof(x->t, off));
}