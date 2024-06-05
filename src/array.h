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
using array_p = rc<array_t>;
using t_arr   = array_p;
#define t_arr_enum T_ARR

struct inter_t;
struct stack_t;
typedef void (*t_ffi)(inter_t* inter, stack_t& s);
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

static const std::string_view type_name_table[T_MAX] = TYPE_ROW("c8", "i64", "f64", "arr", "ffi", "dict");
INLINE const std::string_view type_name(type_t t) { return type_name_table[t]; }

typedef enum flags { FLAG_ATOM = 1, FLAG_QUOTE = 2 } flags_t;
inline flags operator|(flags a, flags b) { return static_cast<flags>(static_cast<int>(a) | static_cast<int>(b)); }
inline flags operator&(flags a, flags b) { return static_cast<flags>(static_cast<int>(a) & static_cast<int>(b)); }

struct array_t {
 private:
  array_t(type_t t, flags_t f, size_t n, array_p owner, void* restrict p) : t(t), f(f), n(n), owner(owner), p(p) {}

  friend class rc<array_t>;
  inline void inc() { rc++; }
  inline void dec() {
    if (!--rc) {
      this->~array_t();
      delete[] (std::byte*)this;
    }
  }

 public:
  type_t  t;
  flags_t f;
  size_t  rc = 1;
  size_t  n;
  array_p owner;
  void* restrict p;

  ~array_t();

  static array_p alloc(type_t t, size_t n, flags_t f);
  static array_p create(type_t t, size_t n, flags_t f, const void* x);
  static array_p create_slice(array_t* x, size_t n, const void* p);
  static array_p atom(type_t t, const void* x) { return create(t, 1, FLAG_ATOM, x); }

  inline array_p alloc_as() const { return array_t::alloc(t, n, f); }
  inline array_p copy() const { return array_t::create(t, n, f, data()); }

  inline const void* data() const { return p; }
};

using array_p = rc<array_t>;

INLINE bool __array_data_simd_aligned(type_t t, size_t n) { return n >= 2 * SIMD_REG_WIDTH_BYTES / type_sizeof(t, 1); }
INLINE bool array_data_simd_aligned(const array_t* a) { return __array_data_simd_aligned(a->t, a->n); }

INLINE const array_t* __array_assert_type(const array_t* a, type_t t) {
  assert(a->t == t);
  return a;
}
INLINE array_t* __array_assert_mut(const array_t* a) {
  assert(a->rc <= 1 && !a->owner);
  return (array_t*)a;
}
INLINE array_t* __array_assert_simd_aligned(array_t* arr) {
  assert(array_data_simd_aligned(arr));
  return arr;
}

INLINE size_t      array_data_sizeof(const array_t* a) { return type_sizeof(a->t, a->n); }
INLINE void*       array_mut_data(array_t* arr) { return (void*)__array_assert_mut(arr)->data(); }
INLINE const void* array_data_i(const array_t* a, size_t i) { return a->data() + type_sizeof(a->t, i); }
INLINE void* array_mut_data_i(const array_t* a, size_t i) { return (void*)array_data_i(__array_assert_mut(a), i); }

#define __DEF_TYPE_HELPER(t)                                                                                          \
  INLINE array_p  array_new_atom_##t(t v) { return array_t::atom(TYPE_ENUM(t), &v); }                                 \
  INLINE array_p  array_new_##t(size_t n, const t* x) { return array_t::create(TYPE_ENUM(t), n, (flags_t)0, x); }     \
  INLINE const t* array_data_##t(const array_t* a) { return (const t*)__array_assert_type(a, TYPE_ENUM(t))->data(); } \
  INLINE t*       array_mut_data_##t(array_t* a) {                                                                    \
    return (t*)array_mut_data((array_t*)__array_assert_type(a, TYPE_ENUM(t)));                                  \
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
#define DO_ARRAY(a, t, i, p)               _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), const t* restrict p = (const t*)a->data())

template <typename Fn>
INLINE void array_for_each_atom(array_p& x, Fn callback) {
  size_t      stride = type_sizeof(x->t, 1);
  const void* ptr    = x->data();
  if (x->t != T_ARR) {
    DO(i, x->n) {
      array_p y = array_t::atom(x->t, ptr + stride * i);
      callback(i, y);
    }
  } else {
    DO_ARRAY(x, t_arr, i, p) callback(i, *p);
  }
}
