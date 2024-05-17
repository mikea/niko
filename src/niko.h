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
#include "str.h"
#include "status.h"
#include "array.h"
#include "inter.h"


// words

#define DEF_WORD_HANDLER(name) STATUS_T name(inter_t* inter, stack_t* stack)

#define DEF_WORD_HANDLER_1_1(n)                                                \
  INLINE RESULT_T n##_impl(const array_t* x);                                  \
  DEF_WORD_HANDLER(n) {                                                        \
    STATUS_CHECK(!stack_is_empty(stack), "stack underflow: 1 value expected"); \
    own(array_t) x = stack_pop(stack);                                         \
    result_t result = n##_impl(x);                                             \
    if (result.ok) {                                                           \
      stack_push(stack, result.either.a);                                      \
      array_dec_ref(result.either.a);                                          \
      return status_ok();                                                      \
    } else {                                                                   \
      return status_err(result.either.e);                                      \
    }                                                                          \
  }                                                                            \
  INLINE RESULT_T n##_impl(const array_t* x)

#define REGISTER_WORD(w, n)                             \
  STATUS_T w_##n(inter_t* inter, stack_t* stack); \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new(str_from_c(w), array_move(array_new_scalar_t_ffi(w_##n))); }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)
