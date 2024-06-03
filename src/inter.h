#pragma once

#include "array.h"
#include "str.h"

#include <ostream>

// token
enum token_type {
  TOK_EOF,
  TOK_ERR,
  TOK_I64,
  TOK_F64,
  TOK_WORD,
  TOK_ARR_OPEN,
  TOK_ARR_CLOSE,
  TOK_STR,
  TOK_QUOTE,
};

struct token_t {
  token_type tok;
  str_t      text;
  union {
    int64_t i;
    double  d;
    str_t   s;
  } val;
};

token_t next_token(const char** s);

// stack

struct stack_t {
  array_t** data;
  size_t    l;
  size_t    cap;
};

INLINE stack_t* stack_new() { return (stack_t*)calloc(1, sizeof(stack_t)); }
INLINE void     stack_clear(stack_t* s) {
  DO(i, s->l) array_dec_ref(s->data[i]);
  s->l = 0;
}
INLINE void stack_free(stack_t* s) {
  stack_clear(s);
  free(s->data);
  free(s);
}
INLINE size_t stack_len(const stack_t* s) { return s->l; }
INLINE bool   stack_is_empty(const stack_t* s) { return !stack_len(s); }
INLINE void   stack_grow(stack_t* s) {
  s->cap  = (s->cap + 1) * 2;
  s->data = (array_t**)reallocarray(s->data, sizeof(array_t*), s->cap);
}
INLINE const array_t* stack_push(stack_t* stack, const array_t* a) {
  if (stack->l == stack->cap) stack_grow(stack);
  return stack->data[stack->l++] = array_inc_ref((array_t*)a);
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  CHECK(stack->l > 0, "stack underflow");
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->data[--stack->l]; }
INLINE void     stack_drop(stack_t* s) { array_dec_ref(stack_pop(s)); }
INLINE          borrow(array_t) stack_peek(stack_t* s, size_t i) {
  CHECK(i < s->l, "stack underflow");
  return s->data[s->l - i - 1];
}
INLINE array_t* stack_i(stack_t* s, size_t i) { return array_inc_ref(stack_peek(s, i)); }

array_t* cat(stack_t* stack, size_t n);

INLINE void __push_back(void* ctx, void* x) { array_dec_ref((array_t*)stack_push((stack_t*)ctx, (array_t*)x)); }

INLINE void array_t_cleanup_protected_push(array_t** p) {
  if (*p) array_t_free(*p);
  *p = NULL;
}

#define POP(x)                       \
  own(array_t) x = stack_pop(stack); \
  defer_catch(stack_push(stack, x))
#define PUSH(x) stack_push(stack, x)
#define DUP     PUSH(stack_peek(stack, 0))
#define DROP    stack_drop(stack)

// dictionary

enum entry_flags { ENTRY_SYS = 1, ENTRY_CONST = 2, ENTRY_VAR = 4, ENTRY_IMM = 8 };
inline entry_flags operator|(entry_flags a, entry_flags b) {
  return static_cast<entry_flags>(static_cast<int>(a) | static_cast<int>(b));
}
inline entry_flags operator&(entry_flags a, entry_flags b) {
  return static_cast<entry_flags>(static_cast<int>(a) & static_cast<int>(b));
}

struct dict_entry_t {
  string_t    k;
  array_t*    v;
  entry_flags f;
};

void global_dict_add_new(dict_entry_t e);

// interpreter

GEN_VECTOR(dict, dict_entry_t)

struct inter_t {
  enum { MODE_INTERPRET, MODE_COMPILE } mode;
  dict_t        dict;
  stack_t*      stack;
  stack_t*      comp_stack;
  string_t      comp;
  size_t        arr_level;
  size_t        arr_marks[256];
  std::ostream* out;
  const char*   line;
};

inter_t* inter_new();
void     inter_free(inter_t* inter);
DEF_CLEANUP(inter_t, inter_free);

inter_t* inter_current();
void     inter_set_current(inter_t* inter);

void inter_load_prelude(inter_t* inter);

void inter_reset(inter_t* inter);
void inter_dict_entry(inter_t* inter, t_dict_entry e);

void        inter_line(inter_t* inter, const char* s);
std::string inter_line_capture_out(inter_t* inter, const char* line);
