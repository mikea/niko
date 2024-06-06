#include <jemalloc/jemalloc.h>
#include <math.h>

#include "niko.h"
#include "print.h"
#include "words.h"

// common utilities

void global_dict_add_ffi1(const char* n, ffi ffi[T_MAX]) {
  auto a = array::create(T_FFI, T_MAX, (flags_t)0, ffi);
  global_dict_add_new({string_t(n), a});
}

void global_dict_add_ffi2(const char* n, ffi ffi[T_MAX][T_MAX]) {
  global_dict_add_new({string_t(n), array::create(T_FFI, T_MAX * T_MAX, (flags_t)0, ffi)});
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

INLINE void thread1(inter_t& inter, stack& stack, const array_p x, ffi ffi_table[T_MAX]) {
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

#define GEN_THREAD1(name, ffi)            \
  DEF_WORD_HANDLER(name##_arr_t) {        \
    POP(x);                               \
    return thread1(inter, stack, x, ffi); \
  }

#pragma region bool

ffi not_table[T_MAX];

GEN_THREAD1(not, not_table);

#define GEN_NOT(t)                                                  \
  DEF_WORD_HANDLER_1_1(not_##t) {                                   \
    array_p out = array::alloc(T_I64, x->n, x->f);                  \
    DO(i, x->n) { out->mut_data<i64_t>()[i] = !(x->data<t>()[i]); } \
    return out;                                                     \
  }

GEN_NOT(i64_t);
GEN_NOT(f64_t);

CONSTRUCTOR void reg_not() {
  not_table[T_I64] = not_i64_t;
  not_table[T_F64] = not_f64_t;
  not_table[T_ARR] = not_arr_t;
  global_dict_add_ffi1("not", not_table);
}
#pragma endregion bool

#pragma region math

// neg

ffi neg_table[T_MAX];

#define GEN_NEG(t)                                            \
  DEF_WORD_HANDLER_1_1(neg_##t) {                             \
    array_p out = x->alloc_as();                              \
    DO(i, x->n) { out->mut_data<t>()[i] = -x->data<t>()[i]; } \
    return out;                                               \
  }

GEN_NEG(i64_t);
GEN_NEG(f64_t);
GEN_THREAD1(neg, neg_table)

CONSTRUCTOR void reg_neg() {
  neg_table[T_I64] = neg_i64_t;
  neg_table[T_F64] = neg_f64_t;
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

#define GEN_SPEC1(name, xt, yt, op)                              \
  DEF_WORD_HANDLER(name##_##xt) {                                \
    POP(x);                                                      \
    array_p y = array::alloc<yt>(x->n, x->f);                    \
    DO(i, x->n) { y->mut_data<yt>()[i] = op(x->data<xt>()[i]); } \
    PUSH(y);                                                     \
    return;                                                      \
  }

#define GEN_FLOAT1_SPEC(name, t, op) GEN_SPEC1(name, t, f64_t, op)

#define GEN_FLOAT(name, op)                    \
  ffi name##_table[T_MAX];                     \
  GEN_FLOAT1_SPEC(name, f64_t, op)             \
  GEN_FLOAT1_SPEC(name, i64_t, op)             \
  GEN_THREAD1(name, name##_table)              \
  CONSTRUCTOR void reg_##name() {              \
    name##_table[T_I64] = name##_i64_t;        \
    name##_table[T_F64] = name##_f64_t;        \
    name##_table[T_ARR] = name##_arr_t;        \
    global_dict_add_ffi1(#name, name##_table); \
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

#define GEN_ROUND(name, op)                    \
  ffi name##_table[T_MAX];                     \
  GEN_SPEC1(name, f64_t, i64_t, op)            \
  GEN_SPEC1(name, i64_t, i64_t, op)            \
  GEN_THREAD1(name, name##_table)              \
  CONSTRUCTOR void reg_##name() {              \
    name##_table[T_I64] = name##_i64_t;        \
    name##_table[T_F64] = name##_f64_t;        \
    name##_table[T_ARR] = name##_arr_t;        \
    global_dict_add_ffi1(#name, name##_table); \
  }

GEN_ROUND(ceil, ceil)
GEN_ROUND(floor, floor)
GEN_ROUND(round, round)
GEN_ROUND(trunc, trunc)

#pragma endregion math

#pragma region binops

typedef void (*binop_kernel_t)(const void* restrict x,
                               size_t x_n,
                               const void* restrict y,
                               size_t y_n,
                               void* restrict out,
                               size_t out_n);

ttT void w_binop(stack& stack, binop_kernel_t kernel) {
  size_t yn = stack.peek(0).n;
  size_t xn = stack.peek(1).n;
  CHECK(yn == xn || yn == 1 || xn == 1, "array lengths are incompatible: {} vs {}", xn, yn);

  POP(y);
  POP(x);

  array_p out = array::alloc<T>(max(xn, yn), x->f & y->f);
  kernel(x->data(), x->n, y->data(), y->n, out->mut_data(), out->n);
  PUSH(out);
}

#define GEN_BINOP_KERNEL(name, a_t, b_t, y_t, op)                                                                 \
  void name(const void* restrict a_ptr, size_t a_n, const void* restrict b_ptr, size_t b_n, void* restrict y_ptr, \
            size_t y_n) {                                                                                         \
    y_t* restrict y       = (y_t*)y_ptr;                                                                          \
    const a_t* restrict a = (a_t*)a_ptr;                                                                          \
    const b_t* restrict b = (b_t*)b_ptr;                                                                          \
    if (y_n == a_n && y_n == b_n) DO(i, y_n) y[i] = op(a[i], b[i]);                                               \
    else if (a_n == 1 && y_n == b_n) DO(i, y_n) y[i] = op(a[0], b[i]);                                            \
    else if (b_n == 1 && y_n == a_n) DO(i, y_n) y[i] = op(a[i], b[0]);                                            \
    else DO(i, y_n) y[i] = op(a[i % a_n], b[i % b_n]);                                                            \
  }

#define GEN_BINOP_SPECIALIZATION(name, a_t, b_t, y_t, op)                   \
  GEN_BINOP_KERNEL(name##_kernel_##a_t##_##b_t, a_t::t, b_t::t, y_t::t, op) \
  DEF_WORD_HANDLER(name##_##a_t##_##b_t) { return w_binop<y_t>(stack, name##_kernel_##a_t##_##b_t); }

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
  array_p y = array::alloc(T_I64, n, (flags_t)0);
  DO_MUT_ARRAY(y, i64_t, i, ptr) { (*ptr) = i; }
  return y;
}

#pragma endregion array_create

#pragma region array_ops

DEF_WORD("reverse", reverse) {
  POP(x);
  array_p y = array::alloc(x->t, x->n, x->f);
  DO(i, x->n) { memcpy(y->mut_data_i(i), x->data_i(x->n - i - 1), type_sizeof(x->t, 1)); }
  PUSH(y);
}

DEF_WORD("take", take) {
  POP(y);
  POP(x);
  size_t  n  = as_size_t(y);
  array_p z  = array::alloc(x->t, n, (flags_t)0);
  size_t  ys = type_sizeof(x->t, x->n);
  DO(i, type_sizeof(x->t, z->n)) { ((char*)z->mut_data())[i] = ((char*)x->data())[i % ys]; }
  PUSH(z);
}

DEF_WORD_1_1("len", len) { return array::atom<i64_t>(x->n); }

DEF_WORD("[]", cell) {
  POP(y);
  CHECK(y->n >= 1, "expected length > 0");
  CHECK(y->t == T_I64, "i64 array expected");
  POP(x);

  size_t  i = WRAP(*y->data<i64_t>(), x->n);
  array_p z;
  if (x->t == T_ARR) z = *(x->data<arr_t>() + i);
  else z = array::atom(x->t, x->data_i(i));
  PUSH(z);
}

DEF_WORD("cat", cat) {
  POP(x);
  array_p y = cat(stack, as_size_t(x));
  PUSH(y);
}

#define GEN_REPEAT_SPECIALIZATION(xt)                                            \
  DEF_WORD_HANDLER(repeat_##xt) {                                                \
    POP(y);                                                                      \
    DO_ARRAY(y, i64_t, i, p) { CHECK(*p >= 0, "non-negative values expected"); } \
    size_t n = 0;                                                                \
    DO_ARRAY(y, i64_t, i, p) { n += *p; }                                        \
    POP(x);                                                                      \
    array_p z   = array::alloc(x->t, n, (flags_t)0);                             \
    auto    dst = z->mut_data<xt>();                                             \
    auto    src = x->data<xt>();                                                 \
    DO_ARRAY(y, i64_t, i, p) {                                                   \
      DO(j, *p) { *dst++ = *(src + i); }                                         \
    }                                                                            \
    PUSH(z);                                                                     \
  }

GEN_REPEAT_SPECIALIZATION(c8_t);
GEN_REPEAT_SPECIALIZATION(i64_t);
GEN_REPEAT_SPECIALIZATION(f64_t);
GEN_REPEAT_SPECIALIZATION(arr_t);

ffi repeat_table[T_MAX][T_MAX] = {};

CONSTRUCTOR void register_repeat() {
  repeat_table[T_C8][T_I64]  = repeat_c8_t;
  repeat_table[T_I64][T_I64] = repeat_i64_t;
  repeat_table[T_F64][T_I64] = repeat_f64_t;
  repeat_table[T_ARR][T_I64] = repeat_arr_t;
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
  POP(op);
  POP(x);

  t_dict_entry e    = as_dict_entry(op);
  auto         iter = [&](size_t i, array_p slice) mutable {
    PUSH(slice);
    if (i > 0) inter.entry(e);
  };
  x->for_each_atom(iter);
}

DEF_WORD(",scan", scan) {
  POP(op);
  POP(x);

  t_dict_entry e = as_dict_entry(op);

  auto iter      = [&](size_t i, array_p slice) mutable {
    if (i > 0) DUP;
    PUSH(slice);
    if (i > 0) inter.entry(e);
  };
  x->for_each_atom(iter);
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",apply", apply) {
  POP(op);
  POP(x);

  t_dict_entry e    = as_dict_entry(op);
  auto         iter = [&](size_t i, array_p slice) mutable {
    PUSH(slice);
    inter.entry(e);
  };
  x->for_each_atom(iter);
  array_p result = cat(stack, x->n);
  PUSH(result);
}

DEF_WORD(",pairwise", pairwise) {
  POP(op);
  POP(x);

  t_dict_entry e = as_dict_entry(op);

  auto iter      = [&](size_t i, array_p slice) mutable {
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
  POP(op);
  POP(n);

  t_dict_entry e = as_dict_entry(op);
  DO(i, as_size_t(n)) { inter.entry(e); }
}

DEF_WORD(",collect", collect) {
  POP(op);
  POP(x);
  t_dict_entry e = as_dict_entry(op);
  size_t       n = as_size_t(x);
  DO(i, n) { inter.entry(e); }
  DROP;
  array_p result = cat(stack, n);
  PUSH(result);
}

DEF_WORD(",trace", trace) {
  POP(op);
  POP(x);

  t_dict_entry e = as_dict_entry(op);
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
  str_t name       = str_t(x->data<c8_t>(), x->n);
  own(char) c_name = str_toc(name);
  own(FILE) file   = fopen(c_name, "r");
  CHECK(file, "failed to open file %s", c_name);
  CHECK(!fseek(file, 0, SEEK_END), "failed to seek file");
  size_t  n = ftell(file);
  array_p y = array::alloc(T_C8, n, (flags_t)0);
  rewind(file);
  size_t read = fread(y->mut_data(), 1, n, file);
  CHECK(n == read, "truncated read");
  return y;
}

#pragma endregion io