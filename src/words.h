#pragma once

#include "inter.h"
#include "print.h"

#define DECL_HANDLER_X(name)                               \
  ttX struct name {                                        \
    inline static void call(inter_t& inter, stack& stack); \
  }

#define DEF_HANDLER_X(name) \
  DECL_HANDLER_X(name);     \
  ttX inline void name<X>::call(inter_t& inter, stack& stack)

#define DECL_HANDLER(name)                                 \
  struct name {                                            \
    inline static void call(inter_t& inter, stack& stack); \
  }

#define DEF_HANDLER(name) inline void name::call(inter_t& inter, stack& stack)

#define DEF_HANDLER_1_1(n)                            \
  INLINE array_p n##_impl(inter_t& inter, array_p x); \
  DEF_HANDLER(n) {                                    \
    POP(x);                                           \
    PUSH(n##_impl(inter, x));                         \
  }                                                   \
  INLINE array_p n##_impl(inter_t& inter, array_p x)

#define REGISTER_WORD(w, n) \
  DECL_HANDLER(w_##n);      \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new({.k = string(w), .v = array::atom<ffi_t>(w_##n::call)}); }

#define REGISTER_IWORD(w, n)                                                                  \
  DECL_HANDLER(w_##n);                                                                        \
  CONSTRUCTOR void __register_w_##n() {                                                       \
    global_dict_add_new({.k = string(w), .v = array::atom<ffi_t>(w_##n::call), .imm = true}); \
  }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_HANDLER(w_##n)

#define DEF_IWORD(w, n) \
  REGISTER_IWORD(w, n)  \
  DEF_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_HANDLER_1_1(w_##n)

INLINE size_t as_size_t(const array* a) {
  CHECK(a->n == 1, "expected single value");
  CHECK(a->t == T_I64, "expected int, got {} instead", a->t);
  i64 i = *a->data<i64_t>();
  CHECK(i >= 0, "expected non-negative value");
  return i;
}

INLINE t_dict_entry as_dict_entry(const array* x) {
  CHECK(x->t == T_DICT_ENTRY, "dict entry expected");
  return *x->data<dict_entry_t>();
}
