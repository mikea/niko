#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "common.h"
#include "status.h"
#include "str.h"

// token
typedef struct {
  enum {
    TOK_EOF,
    TOK_ERR,
    TOK_I64,
    TOK_F64,
    TOK_WORD,
    TOK_ARR_OPEN,
    TOK_ARR_CLOSE,
    TOK_STR,
    TOK_QUOTE,
  } tok;
  str_t text;
  union {
    int64_t i;
    double d;
    str_t s;
  } val;
} token_t;

token_t next_token(const char** s);

// result
typedef struct {
  bool ok;
  union {
    array_t* a;
    string_t e;
  } either;
} result_t;
#define RESULT_T WARN_UNUSED result_t
INLINE result_t result_ok(array_t* a) { return (result_t){.ok = true, .either.a = array_inc_ref(a)}; }
INLINE result_t result_err(string_t msg) { return (result_t){.ok = false, .either.e = msg}; }
INLINE PRINTF(1, 2) result_t result_errf(const char* format, ...) {
  return result_err(VA_ARGS_FWD(format, string_vnewf(format, args)));
}
#define RESULT_UNWRAP(r)                                    \
  ({                                                        \
    result_t __result = (r);                                \
    if (!__result.ok) return status_err(__result.either.e); \
    __result.either.a;                                      \
  })

#define RESULT_OK(a) return result_ok(a)

// stack

typedef struct stack_t stack_t;
struct stack_t {
  array_t** bottom;
  size_t l;
  size_t cap;
};

INLINE stack_t* stack_new() { return calloc(1, sizeof(stack_t)); }
INLINE void stack_clear(stack_t* s) {
  DO(i, s->l) array_dec_ref(s->bottom[i]);
  s->l = 0;
}
INLINE void stack_free(stack_t* s) {
  stack_clear(s);
  free(s->bottom);
  free(s);
}
INLINE size_t stack_len(const stack_t* s) { return s->l; }
INLINE bool stack_is_empty(const stack_t* s) { return !stack_len(s); }
INLINE void stack_grow(stack_t* s) {
  s->cap = (s->cap + 1) * 2;
  s->bottom = reallocarray(s->bottom, sizeof(array_t*), s->cap);
}
INLINE void stack_push(stack_t* stack, const array_t* a) {
  if (stack->l == stack->cap) stack_grow(stack);
  stack->bottom[stack->l++] = array_inc_ref((array_t*)a);
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  assert(stack->l > 0);
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->bottom[--stack->l]; }
INLINE void stack_drop(stack_t* s) { array_dec_ref(stack_pop(s)); }
INLINE array_t* stack_i(stack_t* s, size_t i) {
  assert(i < s->l);
  return array_inc_ref(s->bottom[s->l - i - 1]);
}
INLINE array_t* stack_peek(stack_t* s) { return stack_i(s, 0); }

RESULT_T concatenate(stack_t* stack, shape_t sh);

// dictionary

struct dict_entry_t {
  struct dict_entry_t* n;
  string_t k;
  array_t* v;
};
typedef struct dict_entry_t dict_entry_t;

INLINE dict_entry_t* dict_entry_new(dict_entry_t* n, str_t k, array_t* v) {
  dict_entry_t* e = malloc(sizeof(dict_entry_t));
  e->n = n;
  e->k = str_copy(k);
  e->v = array_inc_ref(v);
  return e;
}
INLINE dict_entry_t* dict_entry_free(dict_entry_t* e) {
  dict_entry_t* n = e->n;
  string_free(e->k);
  array_dec_ref(e->v);
  free(e);
  return n;
}
INLINE void dict_entry_free_chain(dict_entry_t* e) {
  while (e) e = dict_entry_free(e);
}

extern dict_entry_t* global_dict;
INLINE void global_dict_add_new(str_t k, array_t* v) { global_dict = dict_entry_new(global_dict, k, v); }

// interpreter

struct interpreter_t {
  enum { MODE_INTERPRET, MODE_COMPILE } mode;
  dict_entry_t* dict;
  stack_t* stack;
  stack_t* comp_stack;
  string_t comp;
  size_t arr_level;
  size_t arr_marks[256];
  FILE* out;
  const char* line;
};
typedef struct interpreter_t interpreter_t;
RESULT_T interpreter_read_next_word(interpreter_t* inter);
STATUS_T interpreter_dict_entry(interpreter_t* inter, dict_entry_t* e);

// words

#define DEF_WORD_HANDLER(name) STATUS_T name(interpreter_t* inter, stack_t* stack)

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
  STATUS_T w_##n(interpreter_t* inter, stack_t* stack); \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new(str_from_c(w), array_move(array_new_scalar_t_ffi(w_##n))); }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_WORD_HANDLER(w_##n)

#define DEF_WORD_1_1(w, n) \
  REGISTER_WORD(w, n)      \
  DEF_WORD_HANDLER_1_1(w_##n)
