#pragma once

#include "common.h"
#include "simd.h"
#include "status.h"

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

struct interpreter_t;
struct stack_t;
typedef STATUS_T (*t_ffi)(struct interpreter_t* inter, struct stack_t* s);
#define t_ffi_enum T_FFI

struct dict_entry_t;
typedef struct dict_entry_t* t_dict_entry;
#define t_dict_entry_enum T_DICT_ENTRY

#define TYPE_ENUM(t) t##_enum
#define TYPE_SIMD(t) t##_simd

#define TYPE_FOREACH(f) f(t_c8) f(t_i64) f(t_f64) f(t_arr) f(t_ffi) f(t_dict_entry)
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
  size_t r;  // rank
  const dim_t* d;
} shape_t;

INLINE size_t dims_sizeof(size_t r) { return r * sizeof(dim_t); }
INLINE shape_t* shape_new(size_t r) {
  shape_t* s = malloc(sizeof(shape_t) + dims_sizeof(r));
  s->r = r;
  s->d = (dim_t*)(s + 1);
  return s;
}
INLINE void shape_free(shape_t* s) { free(s); }
DEF_CLEANUP(shape_t, shape_free);
INLINE shape_t shape_scalar() { return (shape_t){0, NULL}; }
INLINE shape_t shape_1d(const dim_t* d) { return (shape_t){1, d}; }
INLINE shape_t shape_create(size_t r, const dim_t* d) { return (shape_t){r, d}; }
INLINE bool shape_eq(shape_t s1, shape_t s2) { return s1.r == s2.r && !memcmp(s1.d, s2.d, dims_sizeof(s1.r)); }
INLINE size_t shape_len(shape_t s) {
  size_t y = 1;
  DO(i, s.r) y *= s.d[i];
  return y;
}
INLINE shape_t* shape_extend(shape_t s, dim_t sz) {
  shape_t* r = shape_new(s.r + 1);
  dim_t* d = (dim_t*)r->d;
  memcpy(d + 1, s.d, dims_sizeof(s.r));
  *d = sz;
  return r;
}

// array: (header, dims, data)
typedef struct {
  type_t t;          // type
  size_t rc;         // ref count
  size_t n;          // number of elements
  void* restrict p;  // simd aligned size and length if n is large enough
  size_t r;          // rank
  dim_t d[0];        // dims
} array_t;

INLINE bool __array_data_simd_aligned(type_t t, size_t n) { return n >= 2 * SIMD_REG_WIDTH_BYTES / type_sizeof(t, 1); }
INLINE bool array_data_simd_aligned(const array_t* a) { return __array_data_simd_aligned(a->t, a->n); }

INLINE const dim_t* array_dims(const array_t* a) { return a->d; }
INLINE shape_t array_shape(const array_t* a) { return (shape_t){a->r, array_dims(a)}; }

INLINE const void* array_data(const array_t* arr) { return arr->p; }
INLINE size_t array_data_sizeof(const array_t* a) { return type_sizeof(a->t, a->n); }
INLINE const void* array_data_i(const array_t* a, size_t i) { return array_data(a) + type_sizeof(a->t, i); }

INLINE array_t* __array_assert_mut(array_t* arr) {
  assert(arr->rc == 1);
  return arr;
}
INLINE array_t* __array_assert_simd_aligned(array_t* arr) {
  assert(array_data_simd_aligned(arr));
  return arr;
}

INLINE void* array_mut_data(array_t* arr) { return (void*)array_data(__array_assert_mut(arr)); }

INLINE array_t* array_alloc(type_t t, size_t n, shape_t s) {
  assert(shape_len(s) == n);

  array_t* a;

  if (__array_data_simd_aligned(t, n)) {
    a = malloc(sizeof(array_t) + dims_sizeof(s.r));
    a->p = aligned_alloc(SIMD_REG_WIDTH_BYTES, SIMD_ALIGN_BYTES(type_sizeof(t, n)));
  } else {
    a = malloc(sizeof(array_t) + dims_sizeof(s.r) + type_sizeof(t, n));
    a->p = (void*)(a + 1) + dims_sizeof(s.r);
  }

  a->t = t;
  a->rc = 1;
  a->n = n;
  a->r = s.r;
  memcpy(a + 1, s.d, dims_sizeof(s.r));
  return a;
}

INLINE void array_free(array_t* a) {
  if (__array_data_simd_aligned(a->t, a->n)) free(a->p);
  free(a);
}
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
INLINE array_t* array_new_1d(type_t t, size_t n, const void* x) {
  dim_t d = n;
  return array_new(t, n, shape_1d(&d), x);
}

INLINE bool array_is_scalar(const array_t* a) { return a->r == 0; }
INLINE array_t* array_new_scalar(type_t t, const void* x) { return array_new(t, 1, shape_scalar(), x); }

#define __DEF_TYPE_HELPER(t)                                                                                  \
  INLINE array_t* array_new_scalar_##t(t v) { return array_new_scalar(TYPE_ENUM(t), &v); }                    \
  INLINE array_t* array_new_##t(size_t n, shape_t s, const t* x) { return array_new(TYPE_ENUM(t), n, s, x); } \
  INLINE t* array_data_##t(array_t* a) { return (t*)array_data(a); }

TYPE_FOREACH(__DEF_TYPE_HELPER)

#define __DEF_SIMD_HELPER(t, v)                                         \
  INLINE v* restrict array_mut_data_##v(array_t* a) {                   \
    return (v* restrict)array_mut_data(__array_assert_simd_aligned(a)); \
  }

TYPE_FOREACH_SIMD(__DEF_SIMD_HELPER)

#define __DO_ARRAY_IMPL(a, t, i, p, u)                          \
  for (bool u##b = 1; u##b; u##b = 0)                           \
    for (t* restrict p = (t*)array_mut_data(a); u##b; u##b = 0) \
      for (size_t i = 0, u##n = a->n; i < u##n && u##b; i++, p++)

#define _DO_ARRAY_IMPL(a, t, i, p, u) __DO_ARRAY_IMPL(a, t, i, p, u)
#define DO_ARRAY(a, t, i, p) _DO_ARRAY_IMPL(a, t, i, p, UNIQUE(__))