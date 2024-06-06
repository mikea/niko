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
  DO(i, global_dict.size()) {
    dict_entry& e = global_dict[i];
    dict.push_back({e.k.clone(), e.v, e.f});
  }
  if (prelude) load_prelude();
  global_inter = this;
}

inter_t::~inter_t() {
  assert(global_inter == this);
  global_inter = nullptr;
}

void global_dict_add_new(dict_entry&& e) {
  e.f = e.f | entry_flags::ENTRY_SYS;
  global_dict.push_back(mv(e));
}

array_p cat(stack& stack, size_t n) {
  if (!n) return array::alloc(T_I64, 0, (flags_t)0);
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
    a          = array::alloc(t, n, (flags_t)0);
    void*  ptr = a->mut_data();
    size_t s   = type_sizeof(t, 1);
    DO(i, n) { memcpy(ptr + s * i, stack.peek(n - i - 1).data(), s); }
  } else {
    a = array::alloc(T_ARR, n, (flags_t)0);
    DO_MUT_ARRAY(a, arr_t, i, p) { *p = stack[n - i - 1]; }
  }
  DO(i, n) stack.drop();
  return a;
}

// interpreter

t_dict_entry inter_t::find_entry_idx(const str_t n) {
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

dict_entry* inter_t::find_entry(str_t n) {
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
  if (e->f & ENTRY_CONST || e->f & ENTRY_VAR) {
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
      DO_ARRAY(a, arr_t, i, p) {
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
            else entry(*p);
            break;
          };
        }
      }
      return;
    }
    default: panicf("unexpected type: %d", a->t);
  }
}

str_t inter_t::next_word() {
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
      if (mode == inter_t::INTERPRET || (en->f & ENTRY_IMM)) entry(en);
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
      size_t  l = t.val.s.l;
      array_p a = array::create<c8_t>(l, t.val.s.p);
      PUSH(a);
      return;
    }
    case TOK_QUOTE: {
      size_t e = find_entry_idx(t.val.s);
      CHECK(e < dict.size(), "unknown word '{}'", t.text);
      array_p a = array::atom<dict_entry_t>(e);
      a->f      = a->f | FLAG_QUOTE;
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

std::string inter_t::line_capture_out(const char* l) {
  std::ostringstream buf;
  out = &buf;
  defer { out = &cout; };
  try {
    line(l);
    return buf.str();
  } catch (std::exception& e) { return std::format("ERROR: {}\n", e.what()); }
}

void inter_t::load_prelude() {
  std::string prelude((char*)__prelude_nk, __prelude_nk_len);
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

DEF_WORD_FLAGS(":", def, ENTRY_IMM) {
  CHECK(inter.mode == inter_t::INTERPRET, ": can be used only in interpret mode");
  str_t        next = inter.next_word();
  t_dict_entry prev = inter.find_entry_idx(next);
  if (prev < inter.dict.size()) {
    dict_entry* e = &inter.dict[prev];
    if (e->f & (ENTRY_CONST | ENTRY_SYS)) panicf("`{}` can't be redefined", next);
  }
  inter.mode = inter_t::COMPILE;
  assert(inter.comp_stack.empty());
  inter.comp = next.to_owned();
}

DEF_WORD_FLAGS(";", enddef, ENTRY_IMM) {
  CHECK(inter.mode == inter_t::COMPILE, ": can be used only in compile mode");
  array_p     a    = array::create(T_ARR, inter.comp_stack.len(), (flags_t)0, inter.comp_stack.begin());
  dict_entry* prev = inter.find_entry(inter.comp);
  if (prev) {
    prev->v = a;
    prev->f = (entry_flags)(prev->f & ~ENTRY_VAR);
  } else {
    inter.dict.push_back(dict_entry(inter.comp.clone(), a));
  }

  inter.comp_stack.clear();
  inter.mode = inter_t::INTERPRET;
  inter.comp = nullptr;
}

DEF_WORD_FLAGS("const", _const, ENTRY_IMM) {
  CHECK(inter.mode == inter_t::INTERPRET, "const can be used only in interpret mode");
  str_t next = inter.next_word();
  if (inter.find_entry_idx(next) < inter.dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter.dict.push_back({next.to_owned(), x, ENTRY_CONST});
}

DEF_WORD_FLAGS("var", _var, ENTRY_IMM) {
  CHECK(inter.mode == inter_t::INTERPRET, "var can be used only in interpret mode");
  str_t next = inter.next_word();
  if (inter.find_entry_idx(next) < inter.dict.size()) panicf("`{}` can't be redefined", next);
  POP(x);
  inter.dict.push_back({next.to_owned(), x, ENTRY_VAR});
}

DEF_WORD_FLAGS("literal", literal, ENTRY_IMM) {
  CHECK(inter.mode == inter_t::COMPILE, "literal can be used only in compilation mode");
  POP(x);
  inter.comp_stack.push(x);
}

#pragma endregion immediate

DEF_WORD("!", store) {
  POP(v);
  POP(x);
  dict_entry* e = inter.lookup_entry(as_dict_entry(v));
  CHECK(e->f & ENTRY_VAR || !e->f, "{} is not a valid store target", e->v);
  e->v = x;
  e->f = e->f | ENTRY_VAR;
}

DEF_WORD("@", load) {
  POP(v);
  dict_entry* e = inter.lookup_entry(as_dict_entry(v));
  PUSH(e->v);
}

DEF_WORD("exit", exit) {
  (*inter.out) << "bye\n";
  exit(0);
}

#pragma endregion words