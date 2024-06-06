#pragma once

#include <string.h>

#include "common.h"
#include "memory.h"
#include "simd.h"

struct array;
using array_p = rc<array>;

// type
typedef enum { T_C8, T_I64, T_F64, T_ARR, T_FFI, T_DICT_ENTRY } type_t;
#define T_MAX (T_DICT_ENTRY + 1)

struct c8_t {
  using t                   = char;
  static constexpr type_t e = type_t::T_C8;
};

struct i64_t {
  using t                   = i64;
  static constexpr type_t e = type_t::T_I64;
};

struct f64_t {
  using t                   = f64;
  static constexpr type_t e = type_t::T_F64;
};

struct arr_t {
  using t                   = array_p;
  static constexpr type_t e = type_t::T_ARR;
};

struct inter_t;
struct stack;
typedef void (*ffi)(inter_t& inter, stack& s);
struct ffi_t {
  using t                   = ffi;
  static constexpr type_t e = type_t::T_FFI;
};

typedef i64 t_dict_entry;
struct dict_entry_t {
  using t                   = i64;
  static constexpr type_t e = type_t::T_DICT_ENTRY;
};

#define TYPE_ROW(v_c8, v_i64, v_f64, v_arr, v_ffi, v_dict_entry) \
  { v_c8, v_i64, v_f64, v_arr, v_ffi, v_dict_entry }

#define TYPE_ROW_FOREACH(f) TYPE_ROW(f(c8_t), f(i64_t), f(f64_t), f(arr_t), f(ffi_t), f(dict_entry_t))

#define TYPE_ROW_ID TYPE_ROW(T_C8, T_I64, T_F64, T_ARR, T_FFI)

#define __TYPE_SIZEOF(x) sizeof(x::t)
static size_t type_sizeof_table[T_MAX] = TYPE_ROW_FOREACH(__TYPE_SIZEOF);
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

  inline array* assert_mut() { return assert_this(rc <= 1 && !owner); }

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

  static array_p alloc(type_t t, size_t n, flags_t f);
  static array_p create(type_t t, size_t n, flags_t f, const void* x);
  static array_p create_slice(array* x, size_t n, const void* p);
  static array_p atom(type_t t, const void* x) { return create(t, 1, FLAG_ATOM, x); }

  ttT static inline array_p alloc(size_t n, flags_t f) { return alloc(T::e, n, f); }
  ttT static inline array_p create(size_t n, const T::t* x) { return create(T::e, n, (flags_t)0, x); }
  ttT static inline array_p atom(const T::t& x) { return create(T::e, 1, FLAG_ATOM, &x); }

  inline array_p alloc_as() const { return array::alloc(t, n, f); }
  inline array_p copy() const { return array::create(t, n, f, data()); }

  inline const void* data() const { return p; }
  inline void* restrict mut_data() { return assert_mut()->p; }
  inline const void* data_i(size_t i) const { return data() + type_sizeof(t, i); }
  inline void*       mut_data_i(size_t i) { return mut_data() + type_sizeof(t, i); }

  ttT inline const T::t* data() const { return rcast<const T::t*>(assert_type(T::e)->data()); }
  ttT inline T::t* restrict mut_data() { return rcast<T::t* restrict>(assert_type(T::e)->mut_data()); }

  inline array*       assert_simd_aligned() { return assert_this(simd_aligned()); }
  inline const array* assert_simd_aligned() const { return assert_this(simd_aligned()); }

  inline array*       assert_type(type_t t) { return assert_this(this->t == t); }
  inline const array* assert_type(type_t t) const { return assert_this(this->t == t); }

  template <typename Fn>
  inline void for_each_atom(Fn callback) const;
};

using array_p = rc<array>;

#define __DO_ARRAY_IMPL(a, i, p, u, p_decl) \
  for (bool u##b = 1; u##b; u##b = 0)       \
    for (p_decl; u##b; u##b = 0)            \
      for (size_t i = 0, u##n = a->n; i < u##n && u##b; i++, p++)

#define _DO_ARRAY_IMPL(a, i, p, u, p_decl) __DO_ARRAY_IMPL(a, i, p, u, p_decl)
#define DO_MUT_ARRAY(a, t, i, p)           _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), auto p = a->mut_data<t>())
#define DO_ARRAY(a, t, i, p)               _DO_ARRAY_IMPL(a, i, p, UNIQUE(__), auto p = a->data<t>())

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
    DO_ARRAY(this, arr_t, i, p) callback(i, *p);
  }
}
