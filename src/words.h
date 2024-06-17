#pragma once

#include "inter.h"
#include "print.h"

#define DECL_HANDLER_X(name)                               \
  ttX struct name {                                        \
    inline static void call(inter_t& inter, stack& stack); \
  }

#define DEF_HANDLER_X(name) \
  DECL_HANDLER_X(name);     \
  ttX inline void name<X>::call(inter_t& inter, stack& stack)

#define DECL_HANDLER(name)                                 \
  struct name {                                            \
    inline static void call(inter_t& inter, stack& stack); \
  }

#define DEF_HANDLER(name) inline void name::call(inter_t& inter, stack& stack)

#define REGISTER_WORD(w, n) \
  DECL_HANDLER(w_##n);      \
  CONSTRUCTOR void __register_w_##n() { global_dict_add_new({.k = string(w), .v = array::atom<ffi_t>(w_##n::call)}); }

#define REGISTER_IWORD(w, n)                                                                  \
  DECL_HANDLER(w_##n);                                                                        \
  CONSTRUCTOR void __register_w_##n() {                                                       \
    global_dict_add_new({.k = string(w), .v = array::atom<ffi_t>(w_##n::call), .imm = true}); \
  }

#define DEF_WORD(w, n) \
  REGISTER_WORD(w, n)  \
  DEF_HANDLER(w_##n)

#define DEF_IWORD(w, n) \
  REGISTER_IWORD(w, n)  \
  DEF_HANDLER(w_##n)

INLINE size_t as_size_t(const array* a) {
  CHECK(a->n == 1, "expected single value");
  CHECK(a->t == T_I64, "expected int, got {} instead", a->t);
  i64 i = *a->data<i64_t>();
  CHECK(i >= 0, "expected non-negative value");
  return i;
}
#define _POP_SIZE(v, tmp) \
  POP(tmp);               \
  size_t v = as_size_t(tmp)
#define POP_SIZE(v) _POP_SIZE(v, UNIQUE(v))

INLINE t_dict_entry as_dict_entry(const array* x) {
  CHECK(x->t == T_DICT_ENTRY, "dict entry expected");
  return *x->data<dict_entry_t>();
}

#define _POP_DICT_ENTRY(v, tmp) \
  POP(tmp);                     \
  t_dict_entry v = as_dict_entry(tmp)
#define POP_DICT_ENTRY(v) _POP_DICT_ENTRY(v, UNIQUE(v))
