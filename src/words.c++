#include <jemalloc/jemalloc.h>
#include <math.h>
#include <array>
#include <fstream>
#include <sstream>

#include "niko.h"
#include "print.h"
#include "words.h"

#pragma region support

#pragma region ffi1_support

void global_dict_add_ffi1(str n, const ffi1_table& ffi) {
  global_dict_add_new({string(n), array::create(T_FFI, ffi.size(), ffi.begin())});
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

void global_dict_add_ffi2(str n, const ffi2_table& ffi) {
  global_dict_add_new({string(n), array::create(T_FFI, ffi.size() * ffi.size(), ffi.begin())});
}

template <template <typename, typename> class Func>
struct ffi2_registrar {
  ffi2_registrar(str name, ffi2_table& table) {
    call_each_type2<Func>();
    global_dict_add_ffi2(name, table);
  }
};

#pragma endregion ffi2_support

#pragma endregion support

#pragma region stack

DEF_WORD("dup", dup) { DUP; }

DEF_WORD("swap", swap) {
  POP(a);
  POP(b);
  PUSH(a);
  PUSH(b);
}

DEF_WORD("drop", drop) { stack.drop(); }

DEF_WORD("nip", nip) {
  POP(a);
  DROP;
  PUSH(a);
}

DEF_WORD("over", over) {
  auto a = stack[1];
  PUSH(a);
}

DEF_WORD("rot", rot) {
  POP(a);
  POP(b);
  POP(c);
  PUSH(b);
  PUSH(a);
  PUSH(c);
}

DEF_WORD("tuck", tuck) {
  POP(a);
  POP(b);
  PUSH(a);
  PUSH(b);
  PUSH(a);
}

DEF_WORD("pick", pick) {
  POP(n);
  PUSH(stack[as_size_t(n)]);
}

DEF_WORD("2dup", _2dup) {
  PUSH(stack[1]);
  PUSH(stack[1]);
}

DEF_WORD("2swap", _2swap) {
  POP(y2);
  POP(y1);
  POP(x2);
  POP(x1);
  PUSH(y1);
  PUSH(y2);
  PUSH(x1);
  PUSH(x2);
}

DEF_WORD("2drop", _2drop) {
  DROP;
  DROP;
}

DEF_WORD("2over", _2over) {
  PUSH(stack[3]);
  PUSH(stack[3]);
}

#pragma endregion stack

#pragma region bool

ttX X not_impl(X x) { return !x; }
REG_FN11(not, i64_t, not_impl);

#pragma endregion bool

#pragma region conversions

ttX char c8_impl(X x) { return (char)x; }
REG_FN11(c8, c8_t, c8_impl);

ttX char i64_impl(X x) { return (char)x; }
REG_FN11(i64, i64_t, i64_impl);

ttX char f64_impl(X x) { return (char)x; }
REG_FN11(f64, f64_t, f64_impl);

#pragma endregion conversions

#pragma region math

ttX X neg_impl(X x) { return -x; }
REG_FN11(neg, X, neg_impl);

ttX X abs_impl(X x) { return labs(x); }
template <>
f64 abs_impl<f64>(f64 x) {
  return fabs(x);
}
REG_FN11(abs, X, abs_impl);

REG_FN11(acos, f64_t, acos)
REG_FN11(acosh, f64_t, acosh)
REG_FN11(asin, f64_t, asin)
REG_FN11(asinh, f64_t, asinh)
REG_FN11(atan, f64_t, atan)
REG_FN11(atanh, f64_t, atanh)
REG_FN11(cbrt, f64_t, cbrt)
REG_FN11(cos, f64_t, cos)
REG_FN11(cosh, f64_t, cosh)
REG_FN11(erf, f64_t, erf)
REG_FN11(exp, f64_t, exp)
REG_FN11(bessel1_0, f64_t, j0)
REG_FN11(bessel1_1, f64_t, j1)
REG_FN11(bessel2_0, f64_t, y0)
REG_FN11(bessel2_1, f64_t, y1)
REG_FN11(lgamma, f64_t, lgamma)
REG_FN11(log, f64_t, log)
REG_FN11(log10, f64_t, log10)
REG_FN11(log1p, f64_t, log1p)
REG_FN11(log2, f64_t, log2)
REG_FN11(sin, f64_t, sin)
REG_FN11(sinh, f64_t, sinh)
REG_FN11(sqrt, f64_t, sqrt)
REG_FN11(tan, f64_t, tan)
REG_FN11(tanh, f64_t, tanh)
REG_FN11(ceil, i64_t, ceil)
REG_FN11(floor, i64_t, floor)
REG_FN11(round, i64_t, round)
REG_FN11(trunc, i64_t, trunc)

#pragma endregion math

#pragma region binops

ttXYZ using binop_kernel_t =
    void(*)(const X* restrict x, size_t x_n, const Y* restrict y, size_t y_n, Z* restrict out, size_t out_n);

ttXYZ void w_binop(stack& stack, binop_kernel_t<typename X::t, typename Y::t, typename Z::t> kernel) {
  POP(y);
  POP(x);
  CHECK(y->n == x->n || y->n == 1 || x->n == 1, "array lengths are incompatible: {} vs {}", x->n, y->n);
  array_p z = array::alloc<Z>(max(x->n, y->n));
  z->a      = x->a & y->a;
  kernel(x->data<X>(), x->n, y->data<Y>(), y->n, z->mut_data<Z>(), z->n);
  PUSH(z);
}

#define GEN_BINOP_KERNEL(name, xt, yt, zt, op)                                                             \
  void name(const xt* restrict x, size_t xn, const yt* restrict y, size_t yn, zt* restrict z, size_t zn) { \
    if (zn == xn && zn == yn) DO(i, zn) z[i] = op(x[i], y[i]);                                             \
    else if (xn == 1 && zn == yn) DO(i, zn) z[i] = op(x[0], y[i]);                                         \
    else if (yn == 1 && zn == xn) DO(i, zn) z[i] = op(x[i], y[0]);                                         \
    else DO(i, zn) z[i] = op(x[i % xn], y[i % yn]);                                                        \
  }

#define GEN_BINOP_SPECIALIZATION(name, xt, yt, zt, op)                 \
  GEN_BINOP_KERNEL(name##_kernel_##xt##_##yt, xt::t, yt::t, zt::t, op) \
  DECL_HANDLER(name##_##xt##_##yt);                                    \
  DEF_HANDLER(name##_##xt##_##yt) { return w_binop<xt, yt, zt>(stack, name##_kernel_##xt##_##yt); }

#define GEN_BINOP(word, name, op)                          \
  GEN_BINOP_SPECIALIZATION(name, i64_t, i64_t, i64_t, op)  \
  GEN_BINOP_SPECIALIZATION(name, i64_t, f64_t, f64_t, op)  \
  GEN_BINOP_SPECIALIZATION(name, f64_t, i64_t, f64_t, op)  \
  GEN_BINOP_SPECIALIZATION(name, f64_t, f64_t, f64_t, op)  \
  ffi2_table       name##_table{};                         \
  CONSTRUCTOR void __register_w_##name() {                 \
    name##_table[T_I64][T_I64] = name##_i64_t_i64_t::call; \
    name##_table[T_F64][T_I64] = name##_f64_t_i64_t::call; \
    name##_table[T_I64][T_F64] = name##_i64_t_f64_t::call; \
    name##_table[T_F64][T_F64] = name##_f64_t_f64_t::call; \
    global_dict_add_ffi2(word, name##_table);              \
  }

#define PLUS_OP(a, b) (a) + (b)
GEN_BINOP("+", plus, PLUS_OP)

#define MUL_OP(a, b) (a) * (b)
GEN_BINOP("*", mul, MUL_OP)

#define MINUS_OP(a, b) (a) - (b)
GEN_BINOP("-", minus, MINUS_OP)

#define MAX_OP(a, b) (a) > (b) ? (a) : (b)
GEN_BINOP("|", max, MAX_OP)

#define MIN_OP(a, b) (a) > (b) ? (b) : (a)
GEN_BINOP("&", min, MIN_OP)

#pragma region divide

ffi2_table divide_table{};

#define DIVIDE(x, y) ((f64)(x)) / ((f64)(y))

GEN_BINOP_SPECIALIZATION(divide, i64_t, i64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, i64_t, f64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, f64_t, i64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, f64_t, f64_t, f64_t, DIVIDE)

CONSTRUCTOR void register_divide() {
  divide_table[T_I64][T_I64] = divide_i64_t_i64_t::call;
  divide_table[T_I64][T_F64] = divide_i64_t_f64_t::call;
  divide_table[T_F64][T_I64] = divide_f64_t_i64_t::call;
  divide_table[T_F64][T_F64] = divide_f64_t_f64_t::call;
  global_dict_add_ffi2("/", divide_table);
}

#pragma endregion divide

#pragma region div

ffi2_table div_table{};

#define DIV_INT(x, y)   (x) / (y)
#define DIV_FLOAT(x, y) trunc((x) / (y))

GEN_BINOP_SPECIALIZATION(div, i64_t, i64_t, i64_t, DIV_INT)
GEN_BINOP_SPECIALIZATION(div, i64_t, f64_t, f64_t, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, f64_t, i64_t, f64_t, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, f64_t, f64_t, f64_t, DIV_FLOAT)

CONSTRUCTOR void register_div() {
  div_table[T_I64][T_I64] = div_i64_t_i64_t::call;
  div_table[T_I64][T_F64] = div_i64_t_f64_t::call;
  div_table[T_F64][T_I64] = div_f64_t_i64_t::call;
  div_table[T_F64][T_F64] = div_f64_t_f64_t::call;
  global_dict_add_ffi2("div", div_table);
}

#pragma endregion mod

#pragma region mod

ffi2_table mod_table{};

#define MOD_PERCENT(x, y) ((x) % (y))
#define MOD_FMOD(x, y)    fmod((x), (y))

GEN_BINOP_SPECIALIZATION(mod, i64_t, i64_t, i64_t, MOD_PERCENT)
GEN_BINOP_SPECIALIZATION(mod, i64_t, f64_t, f64_t, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, f64_t, i64_t, f64_t, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, f64_t, f64_t, f64_t, MOD_FMOD)

CONSTRUCTOR void register_mod() {
  mod_table[T_I64][T_I64] = mod_i64_t_i64_t::call;
  mod_table[T_I64][T_F64] = mod_i64_t_f64_t::call;
  mod_table[T_F64][T_I64] = mod_f64_t_i64_t::call;
  mod_table[T_F64][T_F64] = mod_f64_t_f64_t::call;
  global_dict_add_ffi2("mod", mod_table);
}

#pragma endregion mod

#pragma region equal

ffi2_table equal_table{};

#define EQUAL_OP(a, b) (a) == (b)

GEN_BINOP_SPECIALIZATION(equal, c8_t, c8_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, c8_t, i64_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, c8_t, f64_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, i64_t, c8_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, i64_t, i64_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, i64_t, f64_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, f64_t, c8_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, f64_t, i64_t, i64_t, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, f64_t, f64_t, i64_t, EQUAL_OP);

#define REGISTER_EQUAL(t1, t2) equal_table[t1::e][t2::e] = equal_##t1##_##t2::call;

CONSTRUCTOR void register_equal() {
  REGISTER_EQUAL(c8_t, c8_t);
  REGISTER_EQUAL(c8_t, i64_t);
  REGISTER_EQUAL(c8_t, f64_t);
  REGISTER_EQUAL(i64_t, c8_t);
  REGISTER_EQUAL(i64_t, i64_t);
  REGISTER_EQUAL(i64_t, f64_t);
  REGISTER_EQUAL(f64_t, c8_t);
  REGISTER_EQUAL(f64_t, i64_t);
  REGISTER_EQUAL(f64_t, f64_t);
  global_dict_add_ffi2("=", equal_table);
}

#pragma endregion equal

#pragma region less

ffi2_table less_table{};

#define LESS_OP(a, b) (a) < (b)

GEN_BINOP_SPECIALIZATION(less, c8_t, c8_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, c8_t, i64_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, c8_t, f64_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, i64_t, c8_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, i64_t, i64_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, i64_t, f64_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, f64_t, c8_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, f64_t, i64_t, i64_t, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, f64_t, f64_t, i64_t, LESS_OP);

#define REGISTER_less(t1, t2) less_table[t1::e][t2::e] = less_##t1##_##t2::call;

CONSTRUCTOR void register_less() {
  REGISTER_less(c8_t, c8_t);
  REGISTER_less(c8_t, i64_t);
  REGISTER_less(c8_t, f64_t);
  REGISTER_less(i64_t, c8_t);
  REGISTER_less(i64_t, i64_t);
  REGISTER_less(i64_t, f64_t);
  REGISTER_less(f64_t, c8_t);
  REGISTER_less(f64_t, i64_t);
  REGISTER_less(f64_t, f64_t);
  global_dict_add_ffi2("<", less_table);
}

#pragma endregion less

#pragma endregion binops

#pragma region array_create

DEF_WORD_1_1("index", index) {
  size_t  n = as_size_t(x);
  array_p y = array::alloc(T_I64, n);
  DO_MUT_ARRAY(y, i64_t, i, dst) { dst = i; }
  return y;
}

#pragma endregion array_create

#pragma region array_ops

DEF_WORD_1_1("len", len) { return array::atom<i64_t>(x->n); }

ttX struct w_reverse {
  static void call(inter_t& inter, stack& stack) {
    POP(x);
    array_p y = x->alloc_as();
    DO(i, x->n) { y->mut_data<X>()[i] = x->data<X>()[x->n - i - 1]; }
    PUSH(y);
  }
};
ffi1_registrar<w_reverse, c8_t, i64_t, f64_t, arr_t> reverse_registrar("reverse");

ttX struct w_split {
  static array_p split_impl(array_p x, array_p y) {
    if (x->t == T_ARR) {
      array_p z = x->alloc_as();
      DO_MUT_ARRAY(z, arr_t, i, dst) { dst = split_impl(x->data<arr_t>()[i], y); }
      return z;
    }

    CHECK(x->t == y->t, "types are not the same: {} vs {}", x->t, y->t);
    assert(y->n == 1);  // not implemented

    stack  stack;
    auto   needle = y->data<X>()[0];
    size_t s      = 0;
    size_t parts  = 0;
    DO_ARRAY(x, X, i, e) {
      if (e == needle) {
        if (i != s) {
          PUSH(array::create<X>(i - s, x->data<X>() + s));
          parts++;
        }
        s = i + 1;
      }
    }
    if (s != x->n) {
      PUSH(array::create<X>(x->n - s, x->data<X>() + s));
      parts++;
    }

    return cat(stack, parts);
  }

  static void call(inter_t& inter, stack& stack) {
    POP(y);
    POP(x);
    PUSH(split_impl(x, y));
  }
};
ffi1_registrar<w_split, c8_t, i64_t, f64_t> split_registrar("split");

ttX void take(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  size_t  n = as_size_t(y);
  array_p z = array::alloc<X>(n);
  DO(i, n) { z->mut_data<X>()[i] = x->data<X>()[i % x->n]; }
  PUSH(z);
}

ffi2_table take_table;
ttXY struct reg_take {
  reg_take() { take_table[X::e][T_I64] = take<X>; }
};
ffi2_registrar<reg_take> take_registrar("take", take_table);

ttX array_p cell_impl(array_p x, array_p y) {
  switch (y->t) {
    case T_ARR: {
      array_p z = y->alloc_as();
      DO_MUT_ARRAY(z, arr_t, i, dst) { dst = cell_impl<X>(x, y->data<arr_t>()[i]); }
      return z;
    }
    case T_I64: {
      auto ii = y->data<i64_t>();
      if (y->a && X::e == T_ARR) return x->data<arr_t>()[WRAP(ii[0], x->n)];
      array_p z = y->alloc_as<X>();
      DO_MUT_ARRAY(z, X, i, dst) { dst = x->data<X>()[WRAP(ii[i], x->n)]; }
      return z;
    }
    default: panicf("{} is not supported", y->t);
  }
}

ttX void cell(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  PUSH(cell_impl<X>(x, y));
}

ffi2_table cell_table;
ttXY struct reg_cell {
  reg_cell() {
    cell_table[X::e][T_I64] = cell<X>;
    cell_table[X::e][T_ARR] = cell<X>;
  }
};
ffi2_registrar<reg_cell> cell_registrar("[]", cell_table);

DEF_WORD("cat", cat) {
  POP(x);
  PUSH(cat(stack, as_size_t(x)));
}

DEF_WORD("tail", tail) {
  POP(x);
  PUSH(x->tail());
}

ttX void repeat(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  DO_ARRAY(y, i64_t, i, e) { CHECK(e >= 0, "non-negative values expected"); }
  size_t n = 0;
  DO_ARRAY(y, i64_t, i, e) { n += e; }
  array_p z   = array::alloc<X>(n);
  auto    dst = z->mut_data<X>();
  auto    src = x->data<X>();
  DO_ARRAY(y, i64_t, i, e) {
    DO(j, e) { *dst++ = src[i]; }
  }
  PUSH(z);
}

#define REGISTER_REPEAT(t) repeat_table[t::e][T_I64] = repeat<t>;
CONSTRUCTOR void register_repeat() {
  ffi2_table repeat_table{};
  TYPE_FOREACH(REGISTER_REPEAT);
  global_dict_add_ffi2("repeat", repeat_table);
}

ttX struct w_flip {
  static vector<type_t> guess_types(array_p x) {
    if (x->t != T_ARR) return vector<type_t>(x->n, x->t);
    vector<type_t> v;
    DO_ARRAY(x, arr_t, i, e) { v.push_back(e->a ? e->t : T_ARR); }
    return v;
  }

  static array_p flip_impl(array_p x) {
    if (!x->n) return x;

    size_t         w_max = 0;
    size_t         w_min = size_t_max;
    vector<type_t> types = guess_types(x->data<arr_t>()[0]);
    DO_ARRAY(x, arr_t, i, row) {
      w_max = max(w_max, row->n);
      w_min = min(w_min, row->n);
      while (types.size() < w_max) types.push_back(T_ARR);
      vector<type_t> gt = guess_types(row);
      DO(j, gt.size()) {
        if (gt[j] != types[j]) types[j] = T_ARR;
      }
    }
    assert(w_max == types.size());

    vector<array_p> cols;
    DO(i, w_max) { cols.push_back(array::alloc(types[i], x->n)); }

    DO_ARRAY(x, arr_t, i, row) {
      DO(j, cols.size()) {
        if (j >= row->n) {
          assert(types[j] == T_ARR);
          cols[j]->copy_ij(i, array::atom<arr_t>(array::create<arr_t>(0, nullptr)), 0, 1);
          continue;
        }
        if (types[j] == T_ARR)
          if (row->t == T_ARR) cols[j]->copy_ij(i, row, j, 1);
          else cols[j]->copy_ij(i, array::atom<arr_t>(row->atom_i(j)), 0, 1);
        else if (row->t == T_ARR) cols[j]->copy_ij(i, row->data<arr_t>()[j], 0, 1);
        else cols[j]->copy_ij(i, row, j, 1);
      }
    }

    return array::create<arr_t>(cols.size(), &(*cols.begin()));
  }
  static void call(inter_t& inter, stack& stack) {
    POP(x);
    PUSH(flip_impl(x));
  }
};
ffi1_registrar<w_flip, arr_t> flip_registrar("flip");

#pragma endregion array_ops

#pragma region slash_words

DEF_WORD("\\i", slash_info) {
  printf(VERSION_STRING "\n");
  printf("  %-20s %10ld entries\n", "stack size:", stack.len());
  {
    size_t allocated;
    size_t sz = sizeof(allocated);
    CHECK(mallctl("stats.allocated", &allocated, &sz, NULL, 0) == 0, "failed to query heap size");
    printf("  %-20s %10ld bytes\n", "allocated mem:", allocated);
  }
}

DEF_WORD("\\c", slash_clear) { inter.reset(); }
DEF_WORD("\\mem", slash_mem) { malloc_stats_print(NULL, NULL, NULL); }
DEF_WORD("\\s", slash_stack) {
  DO(i, stack.len()) { (*inter.out) << std::format("{}: {}\n", i, stack[i]); }
}

#pragma endregion slash_words

#pragma region adverbs

DEF_WORD(",fold", fold) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(e);
  }
}

DEF_WORD(",scan", scan) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  DO(i, x->n) {
    if (i > 0) DUP;
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(e);
  }
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",apply", apply) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    inter.entry(e);
  };
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",pairwise", pairwise) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(e);
    PUSH(x->atom_i(i));
  };
  DROP;
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",power", power) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  DO(i, as_size_t(x)) { inter.entry(e); }
}

DEF_WORD(",collect", collect) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  size_t       n = as_size_t(x);
  DO(i, n) { inter.entry(e); }
  DROP;
  array_p result = cat(stack, n);
  PUSH(result);
}

DEF_WORD(",trace", trace) {
  POP(y);
  POP(x);
  t_dict_entry e = as_dict_entry(y);
  size_t       n = as_size_t(x);
  DO(i, n) {
    if (i > 0) DUP;
    inter.entry(e);
  }
  PUSH(cat(stack, n));
}

#pragma endregion adverbs

#pragma region io

DEF_WORD(".", dot) {
  POP(x);
  std::println(*inter.out, "{}", x);
}

DEF_WORD_1_1("load_text", load_text) {
  CHECK(x->t == T_C8, "c8 array expected");
  auto               name = string(x->data<c8_t>(), x->n);
  std::ifstream      file(name);
  std::ostringstream buf;
  buf << file.rdbuf();
  auto content = buf.str();
  return array::create<c8_t>(content.size(), content.c_str());
}

#pragma endregion io