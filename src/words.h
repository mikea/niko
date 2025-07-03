#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "array.h"
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

#pragma region ffi1_support

inline void global_dict_add_ffi1(str n, const ffi1_table& ffi) {
  global_dict_add_new({string(n), array::create<ffi_t>(ffi.size(), ffi.begin())});
}

template <template <typename> class Word, typename X, typename... Types>
inline void _reg1(ffi1_table& t) {
  t[X::e] = Word<X>::call;
  _reg1<Word, Types...>(t);
}
template <template <typename> class Word>
inline void _reg1(ffi1_table& t) {}

template <template <typename> class Word, typename... Types>
struct ffi1_registrar {
  ffi1_registrar(str name) {
    ffi1_table table{};
    _reg1<Word, Types...>(table);
    global_dict_add_ffi1(name, table);
  }
};

template <template <typename> class Kernel, typename X, typename... Types>
inline array_p _apply1(array_p x) {
  if (X::e == x->t) {
    using Y   = Kernel<X>::Y;
    array_p y = x->alloc_as<Y>();
    DO(i, x->n) { y->mut_data<Y>()[i] = Kernel<X>::apply(x->data<X>()[i]); }
    return y;
  }
  return _apply1<Kernel, Types...>(x);
}
template <template <typename> class Kernel>
inline array_p _apply1(array_p x) {
  return nullptr;
}

template <template <typename> class Fn, typename... Types>
struct fn11_registrar {
  ttX struct reg {
    reg(ffi1_table& table) { table[X::e] = call; }

    inline static void call(inter_t& inter, stack& stack) {
      POP(x);
      using Y   = Fn<X>::Y;
      array_p y = x->alloc_as<Y>();
      DO(i, x->n) { y->mut_data<Y>()[i] = Fn<X>::apply(x->data<X>()[i]); }
      PUSH(y);
    }
  };

  fn11_registrar(str name) {
    ffi1_table                                table{};
    call_each_arg<reg, ffi1_table&, Types...> _(table);
    table[T_ARR] = thread;
    global_dict_add_ffi1(name, table);
  }

  inline static void thread(inter_t& inter, stack& stack) {
    POP(x);
    PUSH(thread_impl(x));
  }

  inline static array_p thread_impl(array_p x) {
    assert(x->t == T_ARR);
    array_p        y   = x->alloc_as();
    array_p const* src = x->data<arr_t>();
    DO_MUT_ARRAY(y, arr_t, i, dst) {
      dst = src[i]->t == T_ARR ? thread_impl(src[i]) : _apply1<Fn, Types...>(src[i]);
      CHECK(dst, "{} is not supported", src[i]->t);
    }
    return y;
  }
};

#define REG_FN11(name, y_t, fn)                        \
  ttX struct name##_k {                                \
    using Y = y_t;                                     \
    ALWAYS_INLINE Y::t apply(X::t x) { return fn(x); } \
  };                                                   \
  fn11_registrar<name##_k, c8_t, i64_t, f64_t> name##_registrar(#name);

#pragma endregion ffi1_support

#pragma region ffi2_support

INLINE void global_dict_add_ffi2(str n, const ffi2_table& ffi) {
  global_dict_add_new({string(n), array::create<ffi_t>(ffi.size() * ffi.size(), ffi[0].begin())});
}

template <template <typename, typename> class Word, typename Pair, typename... Pairs>
inline void _reg2(ffi2_table& t) {
  using X       = typename std::tuple_element<0, Pair>::type;
  using Y       = typename std::tuple_element<1, Pair>::type;
  t[X::e][Y::e] = Word<X, Y>::call;
  _reg2<Word, Pairs...>(t);
}
template <template <typename, typename> class>
inline void _reg2(ffi2_table& t) {}

template <template <typename, typename> class Word, typename... Pairs>
struct ffi2_registrar {
  ffi2_registrar(str name) {
    ffi2_table table{};
    _reg2<Word, Pairs...>(table);
    global_dict_add_ffi2(name, table);
  }
};

#pragma endregion ffi2_support
