#include "inter.h"
#include "memory.h"
#include "prelude.h"
#include "print.h"
#include "words.h"

#include <iostream>
#include <sstream>

dict_t global_dict;

inter_t* inter_new() {
  inter_t* inter    = (inter_t*)calloc(1, sizeof(inter_t));
  inter->stack      = stack_new();
  inter->comp_stack = stack_new();
  inter->out        = &std::cout;
  dict_reserve(&inter->dict, global_dict.s);
  DO(i, global_dict.s) {
    dict_entry_t* e = &global_dict.d[i];
    dict_push(&inter->dict, (dict_entry_t){string_copy(e->k), array_inc_ref(e->v), e->f});
  }
  return inter;
}

void dict_free(dict_t* dict) {
  DO(i, dict->s) {
    string_free(dict->d[i].k);
    array_dec_ref(dict->d[i].v);
  }
  dict_shrink(dict);
}

void inter_free(inter_t* inter) {
  stack_free(inter->stack);
  stack_free(inter->comp_stack);
  dict_free(&inter->dict);
  free(inter);
}

void global_dict_add_new(dict_entry_t e) {
  dict_push(&global_dict, (dict_entry_t){e.k, e.v, e.f | entry_flags::ENTRY_SYS});
}

DESTRUCTOR void global_dict_free() { dict_free(&global_dict); }

array_t* cat(stack_t* stack, size_t n) {
  if (!n) return array_alloc(T_I64, 0, (flags_t)0);
  CHECK(n <= stack->l, "stack underflow");

  bool same_type      = true;
  borrow(array_t) top = stack_peek(stack, 0);
  const type_t t      = top->t;
  flags_t      f      = top->f;
  DO(i, n) {
    own(array_t) e  = stack_i(stack, i);
    same_type      &= e->t == t;
    f               = f & e->f;
  }

  array_t* a;
  if (same_type && (f & FLAG_ATOM)) {
    assert(t != T_ARR);  // not implemented
    a          = array_alloc(t, n, (flags_t)0);
    void*  ptr = array_mut_data(a);
    size_t s   = type_sizeof(t, 1);
    DO(i, n) { memcpy(ptr + s * i, array_data(stack_peek(stack, n - i - 1)), s); }
  } else {
    a = array_alloc(T_ARR, n, (flags_t)0);
    DO_MUT_ARRAY(a, t_arr, i, p) { *p = stack_i(stack, n - i - 1); }
  }
  DO(i, n) stack_drop(stack);
  return a;
}

// interpreter

token_t _inter_next_token(inter_t* inter) { return next_token(&inter->line); }

t_dict_entry inter_find_entry_idx(inter_t* inter, const str_t n) {
  dict_t* dict = &inter->dict;
  DO(i, dict->s) {
    size_t j = dict->s - i - 1;
    if (str_eq(n, to_str(dict->d[j].k))) return j;
  }
  return dict->s;
}

ALWAYS_INLINE dict_entry_t* inter_lookup_entry(inter_t* inter, t_dict_entry e) {
  CHECK(e < inter->dict.s, "bad dict entry");
  return &inter->dict.d[e];
}

dict_entry_t* inter_find_entry(inter_t* inter, str_t n) {
  size_t e = inter_find_entry_idx(inter, n);
  if (e == inter->dict.s) return NULL;
  return &inter->dict.d[e];
}

void inter_dict_entry(inter_t* inter, t_dict_entry e_idx) {
  stack_t*      stack = inter->stack;
  dict_entry_t* e     = inter_lookup_entry(inter, e_idx);
  if (e->f & ENTRY_CONST || e->f & ENTRY_VAR) {
    PUSH(e->v);
    return;
  }
  array_t* a = e->v;
  // DBG("{} {}", &e->k, a);
  switch (a->t) {
    case T_FFI: {
      t_ffi f;

      switch (a->n) {
        case 1: {
          f = *array_data_t_ffi(a);
          CHECK(f, "not implemented");
          return f(inter, stack);
        }
        case T_MAX: {
          borrow(array_t) x = stack_peek(stack, 0);
          f                 = (array_data_t_ffi(a))[x->t];
          CHECK(f, "{} is not supported", x->t);
          return f(inter, stack);
        }
        case T_MAX* T_MAX: {
          borrow(array_t) y = stack_peek(stack, 0);
          borrow(array_t) x = stack_peek(stack, 1);
          f                 = ((t_ffi(*)[T_MAX])array_data(a))[x->t][y->t];
          CHECK(f, "{} {} are not supported", x->t, y->t);
          return f(inter, stack);
        }
        default: panicf("unexpected ffi length: {}", a->n);
      }
    }
    case T_ARR: {
      DO_ARRAY(a, t_arr, i, p) {
        switch ((*p)->t) {
          case T_C8:
          case T_I64:
          case T_F64: {
            PUSH(*p);
            break;
          }
          case T_ARR:        NOT_IMPLEMENTED;
          case T_FFI:        NOT_IMPLEMENTED;
          case T_DICT_ENTRY: {
            if ((*p)->f & FLAG_QUOTE) PUSH(*p);
            else inter_dict_entry(inter, *array_data_t_dict_entry(*p));
            break;
          };
        }
      }
      return;
    }
    default: panicf("unexpected type: %d", a->t);
  }
}

str_t inter_next_word(inter_t* inter) {
  token_t next = _inter_next_token(inter);
  CHECK(next.tok == TOK_WORD, "word expected");
  return next.text;
}

void inter_token(inter_t* inter, token_t t) {
  stack_t* stack = inter->mode == inter_t::MODE_COMPILE ? inter->comp_stack : inter->stack;

  switch (t.tok) {
    case TOK_EOF:      return;
    case TOK_ERR:      panicf("unexpected token: '{}'", t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = stack->l;
      return;
    }
    case TOK_ARR_CLOSE: {
      CHECK(inter->arr_level, "unbalanced ]");
      size_t mark = inter->arr_marks[--inter->arr_level];
      CHECK(stack->l >= mark, "stack underflow");
      own(array_t) a = cat(stack, stack->l - mark);
      PUSH(a);
      return;
    }
    case TOK_WORD: {
      size_t e = inter_find_entry_idx(inter, t.text);
      CHECK(e < inter->dict.s, "unknown word '{}'", t.text);
      dict_entry_t* entry = inter_lookup_entry(inter, e);

      if (inter->mode == inter_t::MODE_INTERPRET || (entry->f & ENTRY_IMM)) {
        inter_dict_entry(inter, e);
      } else {
        own(array_t) a = array_new_atom_t_dict_entry(e);
        PUSH(a);
      }

      return;
    }
    case TOK_I64: {
      own(array_t) a = array_new_atom_t_i64(t.val.i);
      PUSH(a);
      return;
    }
    case TOK_F64: {
      own(array_t) a = array_new_atom_t_f64(t.val.d);
      PUSH(a);
      return;
    }
    case TOK_STR: {
      size_t l       = t.val.s.l;
      own(array_t) a = array_new_t_c8(l, t.val.s.p);
      PUSH(a);
      return;
    }
    case TOK_QUOTE: {
      size_t e = inter_find_entry_idx(inter, t.val.s);
      CHECK(e < inter->dict.s, "unknown word '{}'", t.text);
      own(array_t) a = array_new_atom_t_dict_entry(e);
      a->f           = a->f | FLAG_QUOTE;
      PUSH(a);
      return;
    }
  }
  UNREACHABLE;
}

inter_t* current;
inter_t* inter_current() {
  assert(current != NULL);
  return current;
}
void inter_reset_current(bool*) { current = NULL; }
void inter_set_current(inter_t* inter) {
  assert(inter != NULL);
  current = inter;
}

void inter_line(inter_t* inter, const char* s) {
  inter->line = s;

  for (;;) {
    token_t t = _inter_next_token(inter);
    if (t.tok == TOK_EOF) return;
    inter_token(inter, t);
  }
}

std::string inter_line_capture_out(inter_t* inter, const char* line) {
  std::ostringstream buf;
  inter->out = &buf;
  defer { inter->out = &std::cout; };
  try {
    inter_line(inter, line);
    return buf.str();
  } catch (std::exception& e) { return std::format("ERROR: {}\n", e.what()); }
}

void inter_load_prelude(inter_t* inter) {
  str_t prelude       = str_new_len((char*)__prelude_nk, __prelude_nk_len);
  own(char) c_prelude = str_toc(prelude);
  inter_line(inter, c_prelude);
}

void inter_reset(inter_t* inter) {
  stack_clear(inter->stack);
  stack_clear(inter->comp_stack);
  inter->arr_level = 0;
  inter->mode      = inter_t::MODE_INTERPRET;
}

#pragma region words

#pragma region immediate

DEF_WORD_FLAGS(":", def, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::MODE_INTERPRET, ": can be used only in interpret mode");
  str_t        next = inter_next_word(inter);
  t_dict_entry prev = inter_find_entry_idx(inter, next);
  if (prev < inter->dict.s) {
    dict_entry_t* e = &inter->dict.d[prev];
    if (e->f & (ENTRY_CONST | ENTRY_SYS)) panicf("`{}` can't be redefined", next);
  }
  inter->mode = inter_t::MODE_COMPILE;
  assert(stack_is_empty(inter->comp_stack));
  inter->comp = str_copy(next);
}

DEF_WORD_FLAGS(";", enddef, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::MODE_COMPILE, ": can be used only in compile mode");
  own(array_t) a     = array_new(T_ARR, inter->comp_stack->l, (flags_t)0, inter->comp_stack->data);
  dict_entry_t* prev = inter_find_entry(inter, to_str(inter->comp));
  if (prev) {
    array_dec_ref(prev->v);
    prev->v = array_inc_ref(a);
    prev->f = (entry_flags)(prev->f & ~ENTRY_VAR);
  } else {
    dict_push(&inter->dict, (dict_entry_t){string_copy(inter->comp), array_inc_ref(a)});
  }

  stack_clear(inter->comp_stack);
  inter->mode = inter_t::MODE_INTERPRET;

  prev        = inter_find_entry(inter, to_str(inter->comp));
  string_free(inter->comp);
}

DEF_WORD_FLAGS("const", _const, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::MODE_INTERPRET, "const can be used only in interpret mode");
  str_t next = inter_next_word(inter);
  if (inter_find_entry_idx(inter, next) < inter->dict.s) panicf("`{}` can't be redefined", next);
  POP(x);
  dict_push(&inter->dict, (dict_entry_t){str_copy(next), array_inc_ref(x), ENTRY_CONST});
}

DEF_WORD_FLAGS("var", _var, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::MODE_INTERPRET, "var can be used only in interpret mode");
  str_t next = inter_next_word(inter);
  if (inter_find_entry_idx(inter, next) < inter->dict.s) panicf("`{}` can't be redefined", next);
  POP(x);
  dict_push(&inter->dict, (dict_entry_t){str_copy(next), array_inc_ref(x), ENTRY_VAR});
}

DEF_WORD_FLAGS("literal", literal, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::MODE_COMPILE, "literal can be used only in compilation mode");
  POP(x);
  stack_push(inter->comp_stack, x);
}

#pragma endregion immediate

DEF_WORD("!", store) {
  POP(v);
  POP(x);
  dict_entry_t* e = inter_lookup_entry(inter, as_dict_entry(v));
  CHECK(e->f & ENTRY_VAR || !e->f, "{} is not a valid store target", v);
  array_dec_ref(e->v);
  e->v = array_inc_ref(x);
  e->f = e->f | ENTRY_VAR;
}

DEF_WORD("@", load) {
  POP(v);
  dict_entry_t* e = inter_lookup_entry(inter, as_dict_entry(v));
  PUSH(e->v);
}

DEF_WORD("exit", exit) {
  (*inter->out) << "bye\n";
  exit(0);
}

#pragma endregion words