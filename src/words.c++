#include <jemalloc/jemalloc.h>
#include <math.h>
#include <fstream>
#include <sstream>

#include "niko.h"
#include "print.h"
#include "words.h"

// common utilities

void global_dict_add_ffi1(const char* n, ffi ffi[T_MAX]) {
  auto a = array::create(T_FFI, T_MAX, ffi);
  global_dict_add_new({string(n), a});
}

void global_dict_add_ffi2(const char* n, ffi ffi[T_MAX][T_MAX]) {
  global_dict_add_new({string(n), array::create(T_FFI, T_MAX * T_MAX, ffi)});
}

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

void thread1(inter_t& inter, stack& stack, ffi ffi_table[T_MAX]) {
  POP(x);
  assert(x->t == T_ARR);
  array_p        out = x->alloc_as();
  array_p const* src = x->data<arr_t>();
  array_p*       dst = out->mut_data<arr_t>();
  DO(i, x->n) {
    PUSH(src[i]);
    ffi f = ffi_table[src[i]->t];
    assert(f);
    f(inter, stack);
    dst[i] = stack.pop();
  }
  PUSH(out);
}

#define GEN_THREAD1(name, ffi_table) \
  DEF_WORD_HANDLER(name##_arr_t) { thread1(inter, stack, ffi_table); }

#pragma region bool

ffi not_table[T_MAX];

GEN_THREAD1(not, not_table);

ttT void w_not(inter_t& inter, stack& stack) {
  POP(x);
  array_p y = x->alloc_as<i64_t>();
  DO(i, x->n) { y->mut_data<i64_t>()[i] = !(x->data<T>()[i]); }
  PUSH(y);
}

CONSTRUCTOR void reg_not() {
  not_table[T_I64] = w_not<i64_t>;
  not_table[T_F64] = w_not<f64_t>;
  not_table[T_ARR] = not_arr_t;
  global_dict_add_ffi1("not", not_table);
}
#pragma endregion bool

#pragma region math

// neg

ffi neg_table[T_MAX];

ttT void w_neg(inter_t& inter, stack& stack) {
  POP(x);
  array_p y = x->alloc_as();
  DO(i, x->n) { y->mut_data<T>()[i] = -x->data<T>()[i]; }
  PUSH(y);
}
GEN_THREAD1(neg, neg_table)

CONSTRUCTOR void reg_neg() {
  neg_table[T_I64] = w_neg<i64_t>;
  neg_table[T_F64] = w_neg<f64_t>;
  neg_table[T_ARR] = neg_arr_t;
  global_dict_add_ffi1("neg", neg_table);
}

// abs

ffi abs_table[T_MAX];

#define GEN_ABS(t, op)                                           \
  DEF_WORD_HANDLER_1_1(abs_##t) {                                \
    array_p out = x->alloc_as();                                 \
    DO(i, x->n) { out->mut_data<t>()[i] = op(x->data<t>()[i]); } \
    return out;                                                  \
  }

GEN_ABS(i64_t, labs);
GEN_ABS(f64_t, fabs);
GEN_THREAD1(abs, abs_table)

CONSTRUCTOR void reg_abs() {
  abs_table[T_I64] = abs_i64_t;
  abs_table[T_F64] = abs_f64_t;
  abs_table[T_ARR] = abs_arr_t;
  global_dict_add_ffi1("abs", abs_table);
}

// sqrt, sin, et. al.
template <typename X, typename Y, typename Op>
void w_math(struct inter_t& inter, class stack& stack) {
  POP(x);
  array_p y = x->alloc_as<Y>();
  DO(i, x->n) { y->mut_data<Y>()[i] = Op::apply(x->data<X>()[i]); }
  PUSH(y);
}

#define GEN_FLOAT(name, fn)                         \
  ffi name##_table[T_MAX];                          \
  GEN_THREAD1(name, name##_table)                   \
  CONSTRUCTOR void reg_##name() {                   \
    struct op {                                     \
      static f64 apply(f64 x) { return fn(x); }     \
    };                                              \
    name##_table[T_I64] = w_math<i64_t, f64_t, op>; \
    name##_table[T_F64] = w_math<f64_t, f64_t, op>; \
    name##_table[T_ARR] = name##_arr_t;             \
    global_dict_add_ffi1(#name, name##_table);      \
  }

GEN_FLOAT(acos, acos)
GEN_FLOAT(acosh, acosh)
GEN_FLOAT(asin, asin)
GEN_FLOAT(asinh, asinh)
GEN_FLOAT(atan, atan)
GEN_FLOAT(atanh, atanh)
GEN_FLOAT(cbrt, cbrt)
GEN_FLOAT(cos, cos)
GEN_FLOAT(cosh, cosh)
GEN_FLOAT(erf, erf)
GEN_FLOAT(exp, exp)
GEN_FLOAT(bessel1_0, j0)
GEN_FLOAT(bessel1_1, j1)
GEN_FLOAT(bessel2_0, y0)
GEN_FLOAT(bessel2_1, y1)
GEN_FLOAT(lgamma, lgamma)
GEN_FLOAT(log, log)
GEN_FLOAT(log10, log10)
GEN_FLOAT(log1p, log1p)
GEN_FLOAT(log2, log2)
GEN_FLOAT(sin, sin)
GEN_FLOAT(sinh, sinh)
GEN_FLOAT(sqrt, sqrt)
GEN_FLOAT(tan, tan)
GEN_FLOAT(tanh, tanh)

#define GEN_ROUND(name, fn)                         \
  ffi name##_table[T_MAX];                          \
  GEN_THREAD1(name, name##_table)                   \
  CONSTRUCTOR void reg_##name() {                   \
    struct op {                                     \
      static i64 apply(f64 x) { return fn(x); }     \
    };                                              \
    name##_table[T_I64] = w_math<i64_t, i64_t, op>; \
    name##_table[T_F64] = w_math<f64_t, i64_t, op>; \
    name##_table[T_ARR] = name##_arr_t;             \
    global_dict_add_ffi1(#name, name##_table);      \
  }

GEN_ROUND(ceil, ceil)
GEN_ROUND(floor, floor)
GEN_ROUND(round, round)
GEN_ROUND(trunc, trunc)

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
  DEF_WORD_HANDLER(name##_##xt##_##yt) { return w_binop<xt, yt, zt>(stack, name##_kernel_##xt##_##yt); }

#define GEN_BINOP(word, name, op)                         \
  GEN_BINOP_SPECIALIZATION(name, i64_t, i64_t, i64_t, op) \
  GEN_BINOP_SPECIALIZATION(name, i64_t, f64_t, f64_t, op) \
  GEN_BINOP_SPECIALIZATION(name, f64_t, i64_t, f64_t, op) \
  GEN_BINOP_SPECIALIZATION(name, f64_t, f64_t, f64_t, op) \
  ffi              name##_table[T_MAX][T_MAX] = {};       \
  CONSTRUCTOR void __register_w_##name() {                \
    name##_table[T_I64][T_I64] = name##_i64_t_i64_t;      \
    name##_table[T_F64][T_I64] = name##_f64_t_i64_t;      \
    name##_table[T_I64][T_F64] = name##_i64_t_f64_t;      \
    name##_table[T_F64][T_F64] = name##_f64_t_f64_t;      \
    global_dict_add_ffi2(word, name##_table);             \
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

ffi divide_table[T_MAX][T_MAX] = {};

#define DIVIDE(x, y) ((f64)(x)) / ((f64)(y))

GEN_BINOP_SPECIALIZATION(divide, i64_t, i64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, i64_t, f64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, f64_t, i64_t, f64_t, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, f64_t, f64_t, f64_t, DIVIDE)

CONSTRUCTOR void register_divide() {
  divide_table[T_I64][T_I64] = divide_i64_t_i64_t;
  divide_table[T_I64][T_F64] = divide_i64_t_f64_t;
  divide_table[T_F64][T_I64] = divide_f64_t_i64_t;
  divide_table[T_F64][T_F64] = divide_f64_t_f64_t;
  global_dict_add_ffi2("/", divide_table);
}

#pragma endregion divide

#pragma region div

ffi div_table[T_MAX][T_MAX] = {};

#define DIV_INT(x, y)   (x) / (y)
#define DIV_FLOAT(x, y) trunc((x) / (y))

GEN_BINOP_SPECIALIZATION(div, i64_t, i64_t, i64_t, DIV_INT)
GEN_BINOP_SPECIALIZATION(div, i64_t, f64_t, f64_t, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, f64_t, i64_t, f64_t, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, f64_t, f64_t, f64_t, DIV_FLOAT)

CONSTRUCTOR void register_div() {
  div_table[T_I64][T_I64] = div_i64_t_i64_t;
  div_table[T_I64][T_F64] = div_i64_t_f64_t;
  div_table[T_F64][T_I64] = div_f64_t_i64_t;
  div_table[T_F64][T_F64] = div_f64_t_f64_t;
  global_dict_add_ffi2("div", div_table);
}

#pragma endregion mod

#pragma region mod

ffi mod_table[T_MAX][T_MAX] = {};

#define MOD_PERCENT(x, y) ((x) % (y))
#define MOD_FMOD(x, y)    fmod((x), (y))

GEN_BINOP_SPECIALIZATION(mod, i64_t, i64_t, i64_t, MOD_PERCENT)
GEN_BINOP_SPECIALIZATION(mod, i64_t, f64_t, f64_t, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, f64_t, i64_t, f64_t, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, f64_t, f64_t, f64_t, MOD_FMOD)

CONSTRUCTOR void register_mod() {
  mod_table[T_I64][T_I64] = mod_i64_t_i64_t;
  mod_table[T_I64][T_F64] = mod_i64_t_f64_t;
  mod_table[T_F64][T_I64] = mod_f64_t_i64_t;
  mod_table[T_F64][T_F64] = mod_f64_t_f64_t;
  global_dict_add_ffi2("mod", mod_table);
}

#pragma endregion mod

#pragma region equal

ffi equal_table[T_MAX][T_MAX] = {};

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

#define REGISTER_EQUAL(t1, t2) equal_table[t1::e][t2::e] = equal_##t1##_##t2;

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

ffi less_table[T_MAX][T_MAX] = {};

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

#define REGISTER_less(t1, t2) less_table[t1::e][t2::e] = less_##t1##_##t2;

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
  DO_MUT_ARRAY(y, i64_t, i, ptr) { (*ptr) = i; }
  return y;
}

#pragma endregion array_create

#pragma region array_ops

ttX void reverse(inter_t& inter, stack& stack) {
  POP(x);
  array_p y = x->alloc_as();
  DO(i, x->n) { y->mut_data<X>()[i] = x->data<X>()[x->n - i - 1]; }
  PUSH(y);
}

#define REGISTER_REVERSE(t) reverse_table[t::e] = reverse<t>;
CONSTRUCTOR void register_reverse() {
  ffi reverse_table[T_MAX] = {};
  TYPE_FOREACH(REGISTER_REVERSE);
  global_dict_add_ffi1("reverse", reverse_table);
}

ttX void take(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  size_t  n = as_size_t(y);
  array_p z = array::alloc<X>(n);
  DO(i, n) { z->mut_data<X>()[i] = x->data<X>()[i % x->n]; }
  PUSH(z);
}

#define REGISTER_TAKE(t) take_table[t::e][T_I64] = take<t>;
CONSTRUCTOR void register_take() {
  ffi take_table[T_MAX][T_MAX] = {};
  TYPE_FOREACH(REGISTER_TAKE);
  global_dict_add_ffi2("take", take_table);
}

DEF_WORD_1_1("len", len) { return array::atom<i64_t>(x->n); }

ttX void cell(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  auto ii = y->data<i64_t>();
  if (y->a && X::e == T_ARR) {
    PUSH(x->data<arr_t>()[WRAP(*ii, x->n)]);
    return;
  }
  array_p z  = y->alloc_as<X>();
  DO_MUT_ARRAY(z, X, i, p) {
    size_t j = WRAP(ii[i], x->n);
    *p       = x->data<X>()[j];
  }
  PUSH(z);
}

#define REGISTER_CELL(t) cell_table[t::e][T_I64] = cell<t>;
CONSTRUCTOR void register_cell() {
  ffi cell_table[T_MAX][T_MAX] = {};
  TYPE_FOREACH(REGISTER_CELL);
  global_dict_add_ffi2("[]", cell_table);
}

DEF_WORD("cat", cat) {
  POP(x);
  PUSH(cat(stack, as_size_t(x)));
}

ttX void repeat(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  DO_ARRAY(y, i64_t, i, p) { CHECK(*p >= 0, "non-negative values expected"); }
  size_t n = 0;
  DO_ARRAY(y, i64_t, i, p) { n += *p; }
  array_p z   = array::alloc<X>(n);
  auto    dst = z->mut_data<X>();
  auto    src = x->data<X>();
  DO_ARRAY(y, i64_t, i, p) {
    DO(j, *p) { *dst++ = src[i]; }
  }
  PUSH(z);
}

#define REGISTER_REPEAT(t) repeat_table[t::e][T_I64] = repeat<t>;
CONSTRUCTOR void register_repeat() {
  ffi repeat_table[T_MAX][T_MAX] = {};
  TYPE_FOREACH(REGISTER_REPEAT);
  global_dict_add_ffi2("repeat", repeat_table);
}

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
  t_dict_entry e    = as_dict_entry(y);
  auto         iter = [&](size_t i, array_p slice) mutable {
    PUSH(slice);
    if (i > 0) inter.entry(e);
  };
  x->for_each_atom(iter);
}

DEF_WORD(",scan", scan) {
  POP(y);
  POP(x);
  t_dict_entry e    = as_dict_entry(y);
  auto         iter = [&](size_t i, array_p slice) mutable {
    if (i > 0) DUP;
    PUSH(slice);
    if (i > 0) inter.entry(e);
  };
  x->for_each_atom(iter);
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",apply", apply) {
  POP(y);
  POP(x);
  t_dict_entry e    = as_dict_entry(y);
  auto         iter = [&](size_t i, array_p slice) mutable {
    PUSH(slice);
    inter.entry(e);
  };
  x->for_each_atom(iter);
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",pairwise", pairwise) {
  POP(y);
  POP(x);
  t_dict_entry e    = as_dict_entry(y);
  auto         iter = [&](size_t i, array_p slice) mutable {
    PUSH(slice);
    if (i > 0) inter.entry(e);
    PUSH(slice);
  };
  x->for_each_atom(iter);
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