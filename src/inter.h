#pragma once

#include "array.h"
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
    double  d;
    str_t   s;
  } val;
} token_t;

token_t next_token(const char** s);

// stack

typedef struct stack_t stack_t;
struct stack_t {
  array_t** data;
  size_t    l;
  size_t    cap;
};

INLINE stack_t* stack_new() { return calloc(1, sizeof(stack_t)); }
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
  s->data = reallocarray(s->data, sizeof(array_t*), s->cap);
}
INLINE const array_t* stack_push(stack_t* stack, const array_t* a) {
  if (stack->l == stack->cap) stack_grow(stack);
  return stack->data[stack->l++] = array_inc_ref((array_t*)a);
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  assert(stack->l > 0);
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->data[--stack->l]; }
INLINE void     stack_drop(stack_t* s) { array_dec_ref(stack_pop(s)); }
INLINE          borrow(array_t) stack_peek(stack_t* s, size_t i) {
  assert(i < s->l);
  return s->data[s->l - i - 1];
}
INLINE array_t* stack_i(stack_t* s, size_t i) { return array_inc_ref(stack_peek(s, i)); }

array_t* concatenate(stack_t* stack, shape_t sh);

INLINE void __push_back(void* ctx, void* x) { array_dec_ref((array_t*)stack_push(ctx, x)); }

INLINE void array_t_cleanup_protected_push(array_t** p) {
  unwind_handler_pop(__push_back, *p);
  if (*p) array_t_free(*p);
  *p = NULL;
}

#define POP(x) \
  CLEANUP(array_t_cleanup_protected_push) array_t* x = PROTECT(array_t, stack_pop(stack), __push_back, stack);
#define PUSH(x) stack_push(stack, x)
#define DUP     PUSH(stack_peek(stack, 0))

// dictionary

struct dict_entry_t {
  struct dict_entry_t* n;
  string_t             k;
  array_t*             v;
};

typedef struct dict_entry_t dict_entry_t;

INLINE dict_entry_t* dict_entry_new(dict_entry_t* n, str_t k, array_t* v) {
  dict_entry_t* e = malloc(sizeof(dict_entry_t));
  e->n            = n;
  e->k            = str_copy(k);
  e->v            = array_inc_ref(v);
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
INLINE void          global_dict_add_new(str_t k, array_t* v) { global_dict = dict_entry_new(global_dict, k, v); }

// interpreter

struct inter_t {
  enum { MODE_INTERPRET, MODE_COMPILE } mode;
  dict_entry_t* dict;
  stack_t*      stack;
  stack_t*      comp_stack;
  string_t      comp;
  size_t        arr_level;
  size_t        arr_marks[256];
  FILE*         out;
  const char*   line;
};
typedef struct inter_t inter_t;

inter_t* inter_new();
void     inter_free(inter_t* inter);
DEF_CLEANUP(inter_t, inter_free);

void inter_load_prelude();

void inter_reset(inter_t* inter);
void inter_dict_entry(inter_t* inter, dict_entry_t* e);
void inter_line(inter_t* inter, const char* s);
void inter_line_capture_out(inter_t* inter, const char* line, char** out, size_t* out_size);