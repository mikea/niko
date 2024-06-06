#pragma once

#include "inter.h"
#include "print.h"

#define DEF_WORD_HANDLER(name) void name(inter_t& inter, stack& stack)

#define DEF_WORD_HANDLER_1_1(n)                       \
  INLINE array_p n##_impl(inter_t& inter, array_p x); \
  DEF_WORD_HANDLER(n) {                               \
    POP(x);                                           \
    array_p y = n##_impl(inter, x);                   \
    PUSH(y);                                          \
  }                                                   \
  INLINE array_p n##_impl(inter_t& inter, array_p x)

#define REGISTER_WORD_FLAGS(w, n, f)                    \
  void             w_##n(inter_t& inter, stack& stack); \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new({string(w), array::atom<ffi_t>(w_##n), f}); }

#define REGISTER_WORD(w, n) REGISTER_WORD_FLAGS(w, n, (entry_flags)0)

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_FLAGS(w, n, f) \
  REGISTER_WORD_FLAGS(w, n, f)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)

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
