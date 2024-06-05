#include "inter.h"
#include "memory.h"
#include "prelude.h"
#include "print.h"
#include "words.h"

#include <iostream>
#include <sstream>

dict_t global_dict;

inter_t::inter_t() {
  dict.reserve(global_dict.size());
  DO(i, global_dict.size()) {
    dict_entry_t& e = global_dict[i];
    dict.push_back({e.k.clone(), e.v, e.f});
  }
}

void global_dict_add_new(dict_entry_t&& e) {
  e.f = e.f | entry_flags::ENTRY_SYS;
  global_dict.push_back(mv(e));
}

array_p cat(stack_t& stack, size_t n) {
  if (!n) return array_t::alloc(T_I64, 0, (flags_t)0);
  CHECK(n <= stack.len(), "stack underflow");

  bool         same_type = true;
  auto&        top       = stack.peek(0);
  const type_t t         = top.t;
  flags_t      f         = top.f;
  DO(i, n) {
    auto e     = stack[i];
    same_type &= e->t == t;
    f          = f & e->f;
  }

  array_p a;
  if (same_type && (f & FLAG_ATOM)) {
    assert(t != T_ARR);  // not implemented
    a          = array_t::alloc(t, n, (flags_t)0);
    void*  ptr = array_mut_data(a);
    size_t s   = type_sizeof(t, 1);
    DO(i, n) { memcpy(ptr + s * i, stack.peek(n - i - 1).data(), s); }
  } else {
    a = array_t::alloc(T_ARR, n, (flags_t)0);
    DO_MUT_ARRAY(a, t_arr, i, p) { *p = stack[n - i - 1]; }
  }
  DO(i, n) stack.drop();
  return a;
}

// interpreter

token_t _inter_next_token(inter_t* inter) { return next_token(&inter->line); }

t_dict_entry inter_find_entry_idx(inter_t* inter, const str_t n) {
  dict_t& dict = inter->dict;
  DO(i, dict.size()) {
    size_t j = dict.size() - i - 1;
    if (n == dict[j].k) return j;
  }
  return dict.size();
}

ALWAYS_INLINE dict_entry_t* inter_lookup_entry(inter_t* inter, t_dict_entry e) {
  CHECK(e < inter->dict.size(), "bad dict entry");
  return &inter->dict[e];
}

dict_entry_t* inter_find_entry(inter_t* inter, str_t n) {
  size_t e = inter_find_entry_idx(inter, n);
  if (e == inter->dict.size()) return NULL;
  return &inter->dict[e];
}

void inter_dict_entry(inter_t* inter, t_dict_entry e_idx) {
  stack_t&      stack = inter->stack;
  dict_entry_t* e     = inter_lookup_entry(inter, e_idx);
  if (e->f & ENTRY_CONST || e->f & ENTRY_VAR) {
    PUSH(e->v);
    return;
  }
  array_t* a = e->v;
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
          auto& x = stack.peek(0);
          f       = (array_data_t_ffi(a))[x.t];
          CHECK(f, "{} is not supported", x.t);
          return f(inter, stack);
        }
        case T_MAX* T_MAX: {
          auto& y = stack.peek(0);
          auto& x = stack.peek(1);
          f       = ((t_ffi(*)[T_MAX])a->data())[x.t][y.t];
          CHECK(f, "{} {} are not supported", x.t, y.t);
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
            else inter_dict_entry(inter, *array_data_t_dict_entry(p->get()));
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
  stack_t& stack = inter->mode == inter_t::COMPILE ? inter->comp_stack : inter->stack;

  switch (t.tok) {
    case TOK_EOF:      return;
    case TOK_ERR:      panicf("unexpected token: '{}'", t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = stack.len();
      return;
    }
    case TOK_ARR_CLOSE: {
      CHECK(inter->arr_level, "unbalanced ]");
      size_t mark = inter->arr_marks[--inter->arr_level];
      CHECK(stack.len() >= mark, "stack underflow");
      array_p a = cat(stack, stack.len() - mark);
      PUSH(a);
      return;
    }
    case TOK_WORD: {
      size_t e = inter_find_entry_idx(inter, t.text);
      CHECK(e < inter->dict.size(), "unknown word '{}'", t.text);
      dict_entry_t* entry = inter_lookup_entry(inter, e);
      if (inter->mode == inter_t::INTERPRET || (entry->f & ENTRY_IMM)) {
        inter_dict_entry(inter, e);
      } else {
        array_p a = array_new_atom_t_dict_entry(e);
        PUSH(a);
      }

      return;
    }
    case TOK_I64: {
      array_p a = array_new_atom_t_i64(t.val.i);
      PUSH(a);
      return;
    }
    case TOK_F64: {
      array_p a = array_new_atom_t_f64(t.val.d);
      PUSH(a);
      return;
    }
    case TOK_STR: {
      size_t  l = t.val.s.l;
      array_p a = array_new_t_c8(l, t.val.s.p);
      PUSH(a);
      return;
    }
    case TOK_QUOTE: {
      size_t e = inter_find_entry_idx(inter, t.val.s);
      CHECK(e < inter->dict.size(), "unknown word '{}'", t.text);
      array_p a = array_new_atom_t_dict_entry(e);
      a->f      = a->f | FLAG_QUOTE;
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
  defer { inter->out = &cout; };
  try {
    inter_line(inter, line);
    return buf.str();
  } catch (std::exception& e) { return std::format("ERROR: {}\n", e.what()); }
}

void inter_load_prelude(inter_t* inter) {
  str_t prelude((char*)__prelude_nk, __prelude_nk_len);
  own(char) c_prelude = str_toc(prelude);
  inter_line(inter, c_prelude);
}

void inter_reset(inter_t* inter) {
  inter->stack.clear();
  inter->comp_stack.clear();
  inter->arr_level = 0;
  inter->mode      = inter_t::INTERPRET;
}

#pragma region words

#pragma region immediate

DEF_WORD_FLAGS(":", def, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::INTERPRET, ": can be used only in interpret mode");
  str_t        next = inter_next_word(inter);
  t_dict_entry prev = inter_find_entry_idx(inter, next);
  if (prev < inter->dict.size()) {
    dict_entry_t* e = &inter->dict[prev];
    if (e->f & (ENTRY_CONST | ENTRY_SYS)) panicf("`{}` can't be redefined", next);
  }
  inter->mode = inter_t::COMPILE;
  assert(inter->comp_stack.empty());
  inter->comp = next.to_owned();
}

DEF_WORD_FLAGS(";", enddef, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::COMPILE, ": can be used only in compile mode");
  array_p       a    = array_t::create(T_ARR, inter->comp_stack.len(), (flags_t)0, inter->comp_stack.begin());
  dict_entry_t* prev = inter_find_entry(inter, inter->comp);
  if (prev) {
    prev->v = a;
    prev->f = (entry_flags)(prev->f & ~ENTRY_VAR);
  } else {
    inter->dict.push_back(dict_entry_t(inter->comp.clone(), a));
  }

  inter->comp_stack.clear();
  inter->mode = inter_t::INTERPRET;
  inter->comp = nullptr;
}

DEF_WORD_FLAGS("const", _const, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::INTERPRET, "const can be used only in interpret mode");
  str_t next = inter_next_word(inter);
  if (inter_find_entry_idx(inter, next) < inter->dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter->dict.push_back({next.to_owned(), x, ENTRY_CONST});
}

DEF_WORD_FLAGS("var", _var, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::INTERPRET, "var can be used only in interpret mode");
  str_t next = inter_next_word(inter);
  if (inter_find_entry_idx(inter, next) < inter->dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter->dict.push_back({next.to_owned(), x, ENTRY_VAR});
}

DEF_WORD_FLAGS("literal", literal, ENTRY_IMM) {
  CHECK(inter->mode == inter_t::COMPILE, "literal can be used only in compilation mode");
  POP(x);
  inter->comp_stack.push(x);
}

#pragma endregion immediate

DEF_WORD("!", store) {
  POP(v);
  POP(x);
  dict_entry_t* e = inter_lookup_entry(inter, as_dict_entry(v));
  CHECK(e->f & ENTRY_VAR || !e->f, "{} is not a valid store target", e->v);
  e->v = x;
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