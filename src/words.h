#pragma once

#include "inter.h"

#define DEF_WORD_HANDLER(name) void name(inter_t* inter, stack_t* stack)

#define DEF_WORD_HANDLER_1_1(n)                                         \
  INLINE array_t* n##_impl(const array_t* x);                           \
  DEF_WORD_HANDLER(n) {                                                 \
    CHECK(!stack_is_empty(stack), "stack underflow: 1 value expected"); \
    own(array_t) x = stack_pop(stack);                                  \
    own(array_t) y = n##_impl(x);                                       \
    stack_push(stack, y);                                               \
  }                                                                     \
  INLINE array_t* n##_impl(const array_t* x)

#define REGISTER_WORD(w, n)                   \
  void w_##n(inter_t* inter, stack_t* stack); \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new(str_from_c(w), array_move(array_new_scalar_t_ffi(w_##n))); }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)
