#include "inter.h"
#include "memory.h"
#include "prelude.h"
#include "print.h"
#include "words.h"

#include <iostream>
#include <sstream>

dict_t global_dict;

inter_t* global_inter;

inter_t::inter_t(bool prelude) {
  assert(!global_inter);
  dict.reserve(global_dict.size());
  for (auto e : global_dict) dict.push_back(e);
  if (prelude) load_prelude();
  global_inter = this;
}

inter_t::~inter_t() {
  assert(global_inter == this);
  global_inter = nullptr;
}

void global_dict_add_new(dict_entry&& e) {
  e.sys = true;
  global_dict.push_back(mv(e));
}

array_p cat(stack& stack, size_t n) {
  if (!n) return array::alloc<i64_t>(0);
  CHECK(n <= stack.len(), "stack underflow");

  bool         same_type = true;
  auto&        top       = stack.peek(0);
  const type_t t         = top.t;
  bool         atom      = top.a;
  DO(i, n) {
    auto e     = stack[i];
    same_type &= e->t == t;
    atom      &= e->a;
  }

  array_p a;
  if (same_type && atom && t != T_ARR) {
    a = array::alloc(t, n);
    DO(i, n) { a->copy_ij(i, stack[n - i - 1], 0, 1); }
  } else {
    a = array::alloc(T_ARR, n);
    DO_MUT_ARRAY(a, arr_t, i, dst) { dst = stack[n - i - 1]; }
  }
  DO(i, n) stack.drop();
  return a;
}

// interpreter

t_dict_entry inter_t::find_entry_idx(const str n) {
  DO(i, dict.size()) {
    size_t j = dict.size() - i - 1;
    if (n == dict[j].k) return j;
  }
  return dict.size();
}

dict_entry* inter_t::lookup_entry(t_dict_entry e) {
  CHECK(e < dict.size(), "bad dict entry");
  return &dict[e];
}

dict_entry* inter_t::find_entry(str n) {
  size_t e = find_entry_idx(n);
  if (e == dict.size()) return NULL;
  return &dict[e];
}

void inter_t::entry(array_p w) {
  CHECK(w->t == T_DICT_ENTRY, "dict entry expected");
  entry(*w->data<dict_entry_t>());
}

void inter_t::entry(t_dict_entry e_idx) { entry(lookup_entry(e_idx)); }

void inter_t::entry(dict_entry* e) {
  if (e->cons || e->var) {
    PUSH(e->v);
    return;
  }
  array* a = e->v;
  switch (a->t) {
    case T_FFI: {
      ffi  f;
      auto d = a->data<ffi_t>();

      switch (a->n) {
        case 1: {
          f = *d;
          CHECK(f, "not implemented");
          return f(*this, stack);
        }
        case T_MAX: {
          auto& x = stack.peek(0);
          f       = d[x.t];
          CHECK(f, "{} is not supported", x.t);
          return f(*this, stack);
        }
        case T_MAX* T_MAX: {
          auto& y = stack.peek(0);
          auto& x = stack.peek(1);
          f       = ((ffi(*)[T_MAX])d)[x.t][y.t];
          CHECK(f, "{} {} are not supported", x.t, y.t);
          return f(*this, stack);
        }
        default: panicf("unexpected ffi length: {}", a->n);
      }
    }
    case T_ARR: {
      DO_ARRAY(a, arr_t, i, e) {
        switch (e->t) {
          case T_C8:
          case T_I64:
          case T_F64: {
            PUSH(e);
            break;
          }
          case T_ARR:        NOT_IMPLEMENTED;
          case T_FFI:        NOT_IMPLEMENTED;
          case T_DICT_ENTRY: {
            if (e->q) PUSH(e);
            else entry(e);
            break;
          };
        }
      }
      return;
    }
    default: panicf("unexpected type: %d", a->t);
  }
}

str inter_t::next_word() {
  token_t next = next_token();
  CHECK(next.tok == TOK_WORD, "word expected");
  return next.text;
}

void inter_t::token(token_t t) {
  auto& stack = mode == inter_t::COMPILE ? comp_stack : this->stack;

  switch (t.tok) {
    case TOK_EOF:      return;
    case TOK_ERR:      panicf("unexpected token: '{}'", t.text);
    case TOK_ARR_OPEN: {
      assert(arr_level < sizeof(arr_marks) / sizeof(arr_marks[0]));
      arr_marks[arr_level++] = stack.len();
      return;
    }
    case TOK_ARR_CLOSE: {
      CHECK(arr_level, "unbalanced ]");
      size_t mark = arr_marks[--arr_level];
      CHECK(stack.len() >= mark, "stack underflow");
      array_p a = cat(stack, stack.len() - mark);
      PUSH(a);
      return;
    }
    case TOK_WORD: {
      size_t e = find_entry_idx(t.text);
      CHECK(e < dict.size(), "unknown word '{}'", t.text);
      dict_entry* en = lookup_entry(e);
      if (mode == INTERPRET || en->imm) entry(en);
      else PUSH(array::atom<dict_entry_t>(e));
      return;
    }
    case TOK_I64: {
      PUSH(array::atom<i64_t>(t.val.i));
      return;
    }
    case TOK_F64: {
      PUSH(array::atom<f64_t>(t.val.d));
      return;
    }
    case TOK_STR: {
      PUSH(array::create<c8_t>(t.val.s.size(), t.val.s.begin()));
      return;
    }
    case TOK_QUOTE: {
      size_t e = find_entry_idx(t.val.s);
      CHECK(e < dict.size(), "unknown word '{}'", t.text);
      array_p a = array::atom<dict_entry_t>(e);
      a->q      = true;
      PUSH(a);
      return;
    }
  }
  UNREACHABLE;
}

inter_t& inter_t::current() {
  assert(global_inter != NULL);
  return *global_inter;
}

void inter_t::line(const char* s) {
  in = s;

  for (;;) {
    token_t t = next_token();
    if (t.tok == TOK_EOF) return;
    token(t);
  }
}

string inter_t::line_capture_out(const char* l) {
  std::ostringstream buf;
  out = &buf;
  defer { out = &cout; };
  try {
    line(l);
    return buf.str();
  } catch (std::exception& e) { return std::format("ERROR: {}\n", e.what()); }
}

void inter_t::load_prelude() {
  string prelude((char*)__prelude_nk, __prelude_nk_len);
  line(prelude.c_str());
}

void inter_t::reset() {
  stack.clear();
  comp_stack.clear();
  arr_level = 0;
  mode      = inter_t::INTERPRET;
}

#pragma region words

#pragma region immediate

DEF_IWORD(":", def) {
  CHECK(inter.mode == inter_t::INTERPRET, ": can be used only in interpret mode");
  str          next = inter.next_word();
  t_dict_entry prev = inter.find_entry_idx(next);
  if (prev < inter.dict.size()) {
    dict_entry* e = &inter.dict[prev];
    if (e->cons || e->sys) panicf("`{}` can't be redefined", next);
  }
  inter.mode = inter_t::COMPILE;
  assert(inter.comp_stack.empty());
  inter.comp = next;
}

DEF_IWORD(";", enddef) {
  CHECK(inter.mode == inter_t::COMPILE, ": can be used only in compile mode");
  array_p     a    = array::create(T_ARR, inter.comp_stack.len(), inter.comp_stack.begin());
  dict_entry* prev = inter.find_entry(inter.comp);
  if (prev) {
    prev->v    = a;
    prev->cons = prev->var = false;
  } else {
    inter.dict.push_back(dict_entry(inter.comp, a));
  }

  inter.comp_stack.clear();
  inter.mode = inter_t::INTERPRET;
  inter.comp.clear();
}

DEF_IWORD("const", _const) {
  CHECK(inter.mode == inter_t::INTERPRET, "const can be used only in interpret mode");
  str next = inter.next_word();
  if (inter.find_entry_idx(next) < inter.dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter.dict.push_back({.k = string(next), .v = x, .cons = true});
}

DEF_IWORD("var", _var) {
  CHECK(inter.mode == inter_t::INTERPRET, "var can be used only in interpret mode");
  str next = inter.next_word();
  if (inter.find_entry_idx(next) < inter.dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter.dict.push_back({.k = string(next), .v = x, .var = true});
}

DEF_IWORD("literal", literal) {
  CHECK(inter.mode == inter_t::COMPILE, "literal can be used only in compilation mode");
  POP(x);
  inter.comp_stack.push(x);
}

#pragma endregion immediate

DEF_WORD("!", store) {
  POP_DICT_ENTRY(y);
  POP(x);
  dict_entry* e = inter.lookup_entry(y);
  CHECK(e->var || (!e->sys && !e->cons), "{} is not a valid store target", e->v);
  e->v   = x;
  e->var = true;
}

DEF_WORD("@", load) {
  POP_DICT_ENTRY(y);
  dict_entry* e = inter.lookup_entry(y);
  PUSH(e->v);
}

DEF_WORD("exit", exit) {
  (*inter.out) << "bye\n";
  exit(0);
}

#pragma endregion words