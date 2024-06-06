#pragma once

#include "array.h"
#include "print.h"
#include "str.h"

#include <iostream>

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
#include <vector>

class stack {
  std::vector<array_p> data;
  stack(const stack&) = delete;
  stack(stack&&)      = delete;

 public:
  stack() {}
  inline size_t  len() const { return data.size(); }
  inline size_t  size() const { return data.size(); }
  inline bool    empty() const { return !len(); }
  inline void    clear() { data.clear(); }
  inline void    push(array_p a) { data.push_back(a); }
  inline array_p pop() {
    CHECK(!empty(), "stack underflow");
    auto b = data.back();
    data.pop_back();
    return b;
  }
  inline void drop() {
    CHECK(!empty(), "stack underflow");
    data.pop_back();
  }
  inline const array& peek(size_t i) {
    CHECK(i < len(), "stack underflow");
    return *data[len() - i - 1];
  }
  inline array_p operator[](size_t i) {
    CHECK(i < len(), "stack underflow");
    return data[len() - i - 1];
  }
  inline array_p* begin() { return &data[0]; }
};

array_p cat(stack& stack, size_t n);

#define POP(x)             \
  array_p x = stack.pop(); \
  defer_catch(stack.push(x))
#define PUSH(x) stack.push(x)
#define DUP     PUSH(stack[0])
#define DROP    stack.drop()

// dictionary

enum entry_flags { ENTRY_SYS = 1, ENTRY_CONST = 2, ENTRY_VAR = 4, ENTRY_IMM = 8 };
inline entry_flags operator|(entry_flags a, entry_flags b) {
  return static_cast<entry_flags>(static_cast<int>(a) | static_cast<int>(b));
}
inline entry_flags operator&(entry_flags a, entry_flags b) {
  return static_cast<entry_flags>(static_cast<int>(a) & static_cast<int>(b));
}

struct dict_entry {
  string_t    k;
  array_p     v;
  entry_flags f;

  inline dict_entry(str_t k, array_p v, entry_flags f = (entry_flags)0) : k(k.to_owned()), v(v), f(f) {}
  dict_entry(const dict_entry&) = delete;
  dict_entry(dict_entry&&)      = default;
};

void global_dict_add_new(dict_entry&& e);

// interpreter

using dict_t = std::vector<dict_entry>;

struct inter_t {
  enum { INTERPRET, COMPILE } mode = INTERPRET;
  dict_t        dict;
  class stack   stack;
  class stack   comp_stack;
  string_t      comp;
  size_t        arr_level = 0;
  size_t        arr_marks[256]{};
  std::ostream* out  = &cout;
  const char*   line = nullptr;

  inter_t();
};

inter_t* inter_current();
void     inter_set_current(inter_t* inter);

void inter_load_prelude(inter_t* inter);

void inter_reset(inter_t* inter);
void inter_dict_entry(inter_t* inter, t_dict_entry e);

void        inter_line(inter_t* inter, const char* s);
std::string inter_line_capture_out(inter_t* inter, const char* line);
