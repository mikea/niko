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
struct c8_t {
  using t                   = char;
  static constexpr type_t e = type_t::T_C8;
};

typedef int64_t t_i64;
#define t_i64_enum T_I64
#define t_i64_simd vmax_i64

struct i64_t {
  using t                   = i64;
  static constexpr type_t e = type_t::T_I64;
};

typedef double t_f64;
#define t_f64_enum T_F64
#define t_f64_simd vmax_f64

struct f64_t {
  using t                   = f64;
  static constexpr type_t e = type_t::T_F64;
};

struct array;
using array_p = rc<array>;
using t_arr   = array_p;
#define t_arr_enum T_ARR
struct array_t {
  using t                   = array_p;
  static constexpr type_t e = type_t::T_ARR;
};

struct inter_t;
struct stack_t;
typedef void (*t_ffi)(inter_t* inter, stack_t& s);
#define t_ffi_enum T_FFI
struct ffi_t {
  using t                   = t_ffi;
  static constexpr type_t e = type_t::T_FFI;
};

typedef i64 t_dict_entry;
#define t_dict_entry_enum T_DICT_ENTRY
struct dict_entry_t {
  using t                   = i64;
  static constexpr type_t e = type_t::T_DICT_ENTRY;
};

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

struct array {
 private:
  array(type_t t, flags_t f, size_t n, array_p owner, void* restrict p) : t(t), f(f), n(n), owner(owner), p(p) {}

  friend class rc<array>;
  inline void inc() { rc++; }
  inline void dec() {
    if (!--rc) {
      this->~array();
      delete[] (std::byte*)this;
    }
  }

  inline array* assert_mut() {
    assert(rc <= 1 && !owner);
    return this;
  }

  static inline bool data_simd_aligned(type_t t, size_t n) { return n >= 2 * SIMD_REG_WIDTH_BYTES / type_sizeof(t, 1); }
  inline bool        simd_aligned() const { return data_simd_aligned(t, n); }

 public:
  type_t  t;
  flags_t f;
  size_t  rc = 1;
  size_t  n;
  array_p owner;
  void* restrict p;

  ~array();

  static array_p     alloc(type_t t, size_t n, flags_t f);
  static array_p     create(type_t t, size_t n, flags_t f, const void* x);
  static array_p     create_slice(array* x, size_t n, const void* p);
  static array_p     atom(type_t t, const void* x) { return create(t, 1, FLAG_ATOM, x); }

  ttT static array_p create(size_t n, const T::t* x) { return create(T::e, n, (flags_t)0, x); }
  ttT static array_p atom(const T::t& x) { return create(T::e, 1, FLAG_ATOM, &x); }

  inline array_p alloc_as() const { return array::alloc(t, n, f); }
  inline array_p copy() const { return array::create(t, n, f, data()); }

  inline void* restrict mut_data() { return assert_mut()->p; }
  inline const void* data() const { return p; }

  inline void*       mut_data_i(size_t i) { return mut_data() + type_sizeof(t, i); }
  inline const void* data_i(size_t i) const { return data() + type_sizeof(t, i); }

  inline array* assert_simd_aligned() {
    assert(simd_aligned());
    return this;
  }

  inline const array* assert_simd_aligned() const {
    assert(simd_aligned());
    return this;
  }

  inline array* assert_type(type_t t) {
    assert(this->t == t);
    return this;
  }

  inline const array* assert_type(type_t t) const {
    assert(this->t == t);
    return this;
  }

  template <typename Fn>
  inline void for_each_atom(Fn callback) const;
};

using array_p = rc<array>;

#define __DEF_TYPE_HELPER(t)                                                                                \
  INLINE const t* array_data_##t(const array* a) { return (const t*)a->assert_type(TYPE_ENUM(t))->data(); } \
  INLINE t*       array_mut_data_##t(array* a) { return (t*)a->assert_type(TYPE_ENUM(t))->mut_data(); }

TYPE_FOREACH(__DEF_TYPE_HELPER)

#define __DEF_SIMD_HELPER(t, v) \
  INLINE v* restrict array_mut_data_##v(array* a) { return (v* restrict)a->assert_simd_aligned()->mut_data(); }

TYPE_FOREACH_SIMD(__DEF_SIMD_HELPER)

#define __DO_ARRAY_IMPL(a, i, p, u, p_decl) \
  for (bool u##b = 1; u##b; u##b = 0)       \
    for (p_decl; u##b; u##b = 0)            \
      for (size_t i = 0, u##n = a->n; i < u##n && u##b; i++, p++)

#define _DO_ARRAY_IMPL(a, i, p, u, p_decl) __DO_ARRAY_IMPL(a, i, p, u, p_decl)
#define DO_MUT_ARRAY(a, t, i, p)           _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), t* restrict p = (t*)a->mut_data())
#define DO_ARRAY(a, t, i, p)               _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), const t* restrict p = (const t*)a->data())

template <typename Fn>
inline void array::for_each_atom(Fn callback) const {
  size_t      stride = type_sizeof(t, 1);
  const void* ptr    = data();
  if (t != T_ARR) {
    DO(i, n) {
      array_p y = array::atom(t, ptr + stride * i);
      callback(i, y);
    }
  } else {
    DO_ARRAY(this, t_arr, i, p) callback(i, *p);
  }
}
