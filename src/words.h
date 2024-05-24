#pragma once

#include "inter.h"

#define DEF_WORD_HANDLER(name) void name(inter_t* inter, stack_t* stack)

#define DEF_WORD_HANDLER_1_1(n)               \
  INLINE array_t* n##_impl(const array_t* x); \
  DEF_WORD_HANDLER(n) {                       \
    POP(x);                                   \
    own(array_t) y = n##_impl(x);             \
    PUSH(y);                                  \
  }                                           \
  INLINE array_t* n##_impl(const array_t* x)

#define REGISTER_WORD(w, n)                                                               \
  void             w_##n(inter_t* inter, stack_t* stack);                                 \
  CONSTRUCTOR void __register_w_##n() {                                                   \
    global_dict_add_new((dict_entry_t){string_from_c(w), array_new_scalar_t_ffi(w_##n)}); \
  }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)

INLINE size_t as_size_t(array_t* a) {
  CHECK(a->r == 0, "scalar expected");
  CHECK(a->t == T_I64, "int scalar expected");
  i64 i = *array_data_t_i64(a);
  CHECK(i >= 0, "non-negative scalar expected");
  return i;
}

INLINE t_dict_entry as_dict_entry(array_t* x) {
  CHECK(x->t == T_DICT_ENTRY, "dict entry expected");
  return *array_data_t_dict_entry(x);
}
