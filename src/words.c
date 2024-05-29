#include <jemalloc/jemalloc.h>
#include <math.h>

#include "niko.h"
#include "words.h"

// common utilities

void global_dict_add_ffi1(const char* n, t_ffi ffi[T_MAX]) {
  size_t d = T_MAX;
  global_dict_add_new((dict_entry_t){string_from_c(n), array_new(T_FFI, T_MAX, shape_1d(&d), ffi)});
}

void global_dict_add_ffi2(const char* n, t_ffi ffi[T_MAX][T_MAX]) {
  size_t dims[2] = {T_MAX, T_MAX};
  global_dict_add_new((dict_entry_t){string_from_c(n), array_new(T_FFI, T_MAX * T_MAX, (shape_t){2, dims}, ffi)});
}

#pragma region stack

DEF_WORD("dup", dup) { DUP; }

DEF_WORD("swap", swap) {
  POP(a);
  POP(b);
  PUSH(a);
  PUSH(b);
}

DEF_WORD("drop", drop) { stack_drop(stack); }

DEF_WORD("nip", nip) {
  POP(a);
  DROP;
  PUSH(a);
}

DEF_WORD("over", over) {
  own(array_t) a = stack_i(stack, 1);
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
  PUSH(array_move(stack_i(stack, as_size_t(n))));
}

#pragma endregion stack

INLINE void thread1(inter_t* inter, stack_t* stack, const array_t* x, t_ffi ffi_table[T_MAX]) {
  assert(x->t == T_ARR);
  own(array_t) out    = array_alloc_as(x);

  array_t* const* src = array_data_t_arr(x);
  array_t**       dst = array_mut_data_t_arr(out);
  DO(i, x->n) {
    PUSH(src[i]);
    t_ffi ffi = ffi_table[src[i]->t];
    assert(ffi);
    ffi(inter, stack);
    dst[i] = stack_pop(stack);
  }
  PUSH(out);
}

#define GEN_THREAD1(name, ffi)            \
  DEF_WORD_HANDLER(name##_t_arr) {        \
    POP(x);                               \
    return thread1(inter, stack, x, ffi); \
  }

#pragma region bool

t_ffi not_table[T_MAX];

GEN_THREAD1(not, not_table);

#define GEN_NOT(t)                                                                        \
  DEF_WORD_HANDLER_1_1(not_##t) {                                                         \
    own(array_t) out                          = array_alloc(T_I64, x->n, array_shape(x)); \
    DO(i, x->n)(array_mut_data_t_i64(out))[i] = !((const t*)array_data(x))[i];            \
    return array_inc_ref(out);                                                            \
  }

GEN_NOT(t_i64);
GEN_NOT(t_f64);

CONSTRUCTOR void reg_not() {
  not_table[T_I64] = not_t_i64;
  not_table[T_F64] = not_t_f64;
  not_table[T_ARR] = not_t_arr;
  global_dict_add_ffi1("not", not_table);
}
#pragma endregion bool

#pragma region math

// neg

t_ffi neg_table[T_MAX];

#define GEN_NEG(t)                                                           \
  DEF_WORD_HANDLER_1_1(neg_##t) {                                            \
    own(array_t) out                        = array_alloc_as(x);             \
    DO(i, x->n)((t*)array_mut_data(out))[i] = -((const t*)array_data(x))[i]; \
    return array_inc_ref(out);                                               \
  }

GEN_NEG(t_i64);
GEN_NEG(t_f64);
GEN_THREAD1(neg, neg_table)

CONSTRUCTOR void reg_neg() {
  neg_table[T_I64] = neg_t_i64;
  neg_table[T_F64] = neg_t_f64;
  neg_table[T_ARR] = neg_t_arr;
  global_dict_add_ffi1("neg", neg_table);
}

// abs

t_ffi abs_table[T_MAX];

#define GEN_ABS(t, op)                                                          \
  DEF_WORD_HANDLER_1_1(abs_##t) {                                               \
    own(array_t) out                        = array_alloc_as(x);                \
    DO(i, x->n)((t*)array_mut_data(out))[i] = op(((const t*)array_data(x))[i]); \
    return array_inc_ref(out);                                                  \
  }

GEN_ABS(t_i64, labs);
GEN_ABS(t_f64, fabs);
GEN_THREAD1(abs, abs_table)

CONSTRUCTOR void reg_abs() {
  abs_table[T_I64] = abs_t_i64;
  abs_table[T_F64] = abs_t_f64;
  abs_table[T_ARR] = abs_t_arr;
  global_dict_add_ffi1("abs", abs_table);
}

// sqrt, sin, et. al.

#define GEN_FLOAT1_SPEC(name, t, op)                                                        \
  DEF_WORD_HANDLER(name##_##t) {                                                            \
    POP(x);                                                                                 \
    own(array_t) out                            = array_alloc(T_F64, x->n, array_shape(x)); \
    DO(i, x->n)((t_f64*)array_mut_data(out))[i] = op(((const t*)array_data(x))[i]);         \
    PUSH(out);                                                                              \
    return;                                                                                 \
  }

#define GEN_FLOAT(name, op)                    \
  t_ffi name##_table[T_MAX];                   \
  GEN_FLOAT1_SPEC(name, t_f64, op)             \
  GEN_FLOAT1_SPEC(name, t_i64, op)             \
  GEN_THREAD1(name, name##_table)              \
  CONSTRUCTOR void reg_##name() {              \
    name##_table[T_I64] = name##_t_i64;        \
    name##_table[T_F64] = name##_t_f64;        \
    name##_table[T_ARR] = name##_t_arr;        \
    global_dict_add_ffi1(#name, name##_table); \
  }

GEN_FLOAT(acos, acos)
GEN_FLOAT(acosh, acosh)
GEN_FLOAT(asin, asin)
GEN_FLOAT(asinh, asinh)
GEN_FLOAT(atan, atan)
GEN_FLOAT(atanh, atanh)
GEN_FLOAT(cbrt, cbrt)
GEN_FLOAT(ceil, ceil)
GEN_FLOAT(cos, cos)
GEN_FLOAT(cosh, cosh)
GEN_FLOAT(erf, erf)
GEN_FLOAT(exp, exp)
GEN_FLOAT(floor, floor)
GEN_FLOAT(bessel1_0, j0)
GEN_FLOAT(bessel1_1, j1)
GEN_FLOAT(bessel2_0, y0)
GEN_FLOAT(bessel2_1, y1)
GEN_FLOAT(lgamma, lgamma)
GEN_FLOAT(log, log)
GEN_FLOAT(log10, log10)
GEN_FLOAT(log1p, log1p)
GEN_FLOAT(log2, log2)
GEN_FLOAT(round, round)
GEN_FLOAT(sin, sin)
GEN_FLOAT(sinh, sinh)
GEN_FLOAT(sqrt, sqrt)
GEN_FLOAT(tan, tan)
GEN_FLOAT(tanh, tanh)
GEN_FLOAT(trunc, trunc)

#pragma endregion math

#pragma region binops

typedef void (*binop_kernel_t)(const void* restrict x,
                               size_t x_n,
                               const void* restrict y,
                               size_t y_n,
                               void* restrict out,
                               size_t out_n);

void w_binop(stack_t* stack, type_t t, binop_kernel_t kernel) {
  shape_t ys = array_shape(stack_peek(stack, 0));
  shape_t xs = array_shape(stack_peek(stack, 1));
  CHECK(shapes_are_compatible(xs, ys), "array shapes are incompatible: %pH vs %pH", &xs, &ys);

  POP(y);
  POP(x);

  own(array_t) out = array_alloc_shape(t, shape_max(xs, ys));
  kernel(array_data(x), x->n, array_data(y), y->n, array_mut_data(out), out->n);
  PUSH(out);
}

#define GEN_BINOP_KERNEL(name, a_t, b_t, y_t, op)                                                                 \
  void name(const void* restrict a_ptr, size_t a_n, const void* restrict b_ptr, size_t b_n, void* restrict y_ptr, \
            size_t y_n) {                                                                                         \
    y_t* restrict y       = y_ptr;                                                                                \
    const a_t* restrict a = a_ptr;                                                                                \
    const b_t* restrict b = b_ptr;                                                                                \
    if (y_n == a_n && y_n == b_n) DO(i, y_n) y[i] = op(a[i], b[i]);                                               \
    else if (a_n == 1 && y_n == b_n) DO(i, y_n) y[i] = op(a[0], b[i]);                                            \
    else if (b_n == 1 && y_n == a_n) DO(i, y_n) y[i] = op(a[i], b[0]);                                            \
    else DO(i, y_n) y[i] = op(a[i % a_n], b[i % b_n]);                                                            \
  }

#define GEN_BINOP_SPECIALIZATION(name, a_t, b_t, y_t, op)          \
  GEN_BINOP_KERNEL(name##_kernel_##a_t##_##b_t, a_t, b_t, y_t, op) \
  DEF_WORD_HANDLER(name##_##a_t##_##b_t) { return w_binop(stack, TYPE_ENUM(y_t), name##_kernel_##a_t##_##b_t); }

#define GEN_BINOP(word, name, op)                         \
  GEN_BINOP_SPECIALIZATION(name, t_i64, t_i64, t_i64, op) \
  GEN_BINOP_SPECIALIZATION(name, t_i64, t_f64, t_f64, op) \
  GEN_BINOP_SPECIALIZATION(name, t_f64, t_i64, t_f64, op) \
  GEN_BINOP_SPECIALIZATION(name, t_f64, t_f64, t_f64, op) \
  t_ffi            name##_table[T_MAX][T_MAX] = {};       \
  CONSTRUCTOR void __register_w_##name() {                \
    name##_table[T_I64][T_I64] = name##_t_i64_t_i64;      \
    name##_table[T_F64][T_I64] = name##_t_f64_t_i64;      \
    name##_table[T_I64][T_F64] = name##_t_i64_t_f64;      \
    name##_table[T_F64][T_F64] = name##_t_f64_t_f64;      \
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

t_ffi divide_table[T_MAX][T_MAX] = {};

#define DIVIDE(x, y) ((f64)(x)) / ((f64)(y))

GEN_BINOP_SPECIALIZATION(divide, t_i64, t_i64, t_f64, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, t_i64, t_f64, t_f64, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, t_f64, t_i64, t_f64, DIVIDE)
GEN_BINOP_SPECIALIZATION(divide, t_f64, t_f64, t_f64, DIVIDE)

CONSTRUCTOR void register_divide() {
  divide_table[T_I64][T_I64] = divide_t_i64_t_i64;
  divide_table[T_I64][T_F64] = divide_t_i64_t_f64;
  divide_table[T_F64][T_I64] = divide_t_f64_t_i64;
  divide_table[T_F64][T_F64] = divide_t_f64_t_f64;
  global_dict_add_ffi2("/", divide_table);
}

#pragma endregion divide

#pragma region div

t_ffi div_table[T_MAX][T_MAX] = {};

#define DIV_INT(x, y)   (x) / (y)
#define DIV_FLOAT(x, y) trunc((x) / (y))

GEN_BINOP_SPECIALIZATION(div, t_i64, t_i64, t_i64, DIV_INT)
GEN_BINOP_SPECIALIZATION(div, t_i64, t_f64, t_f64, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, t_f64, t_i64, t_f64, DIV_FLOAT)
GEN_BINOP_SPECIALIZATION(div, t_f64, t_f64, t_f64, DIV_FLOAT)

CONSTRUCTOR void register_div() {
  div_table[T_I64][T_I64] = div_t_i64_t_i64;
  div_table[T_I64][T_F64] = div_t_i64_t_f64;
  div_table[T_F64][T_I64] = div_t_f64_t_i64;
  div_table[T_F64][T_F64] = div_t_f64_t_f64;
  global_dict_add_ffi2("div", div_table);
}

#pragma endregion mod

#pragma region mod

t_ffi mod_table[T_MAX][T_MAX] = {};

#define MOD_PERCENT(x, y) ((x) % (y))
#define MOD_FMOD(x, y)    fmod((x), (y))

GEN_BINOP_SPECIALIZATION(mod, t_i64, t_i64, t_i64, MOD_PERCENT)
GEN_BINOP_SPECIALIZATION(mod, t_i64, t_f64, t_f64, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, t_f64, t_i64, t_f64, MOD_FMOD)
GEN_BINOP_SPECIALIZATION(mod, t_f64, t_f64, t_f64, MOD_FMOD)

CONSTRUCTOR void register_mod() {
  mod_table[T_I64][T_I64] = mod_t_i64_t_i64;
  mod_table[T_I64][T_F64] = mod_t_i64_t_f64;
  mod_table[T_F64][T_I64] = mod_t_f64_t_i64;
  mod_table[T_F64][T_F64] = mod_t_f64_t_f64;
  global_dict_add_ffi2("mod", mod_table);
}

#pragma endregion mod

#pragma region equal

t_ffi equal_table[T_MAX][T_MAX] = {};

#define EQUAL_OP(a, b) (a) == (b)

GEN_BINOP_SPECIALIZATION(equal, t_c8, t_c8, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_c8, t_i64, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_c8, t_f64, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_i64, t_c8, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_i64, t_i64, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_i64, t_f64, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_f64, t_c8, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_f64, t_i64, t_i64, EQUAL_OP);
GEN_BINOP_SPECIALIZATION(equal, t_f64, t_f64, t_i64, EQUAL_OP);

#define REGISTER_EQUAL(t1, t2) equal_table[TYPE_ENUM(t1)][TYPE_ENUM(t2)] = equal_##t1##_##t2;

CONSTRUCTOR void register_equal() {
  REGISTER_EQUAL(t_c8, t_c8);
  REGISTER_EQUAL(t_c8, t_i64);
  REGISTER_EQUAL(t_c8, t_f64);
  REGISTER_EQUAL(t_i64, t_c8);
  REGISTER_EQUAL(t_i64, t_i64);
  REGISTER_EQUAL(t_i64, t_f64);
  REGISTER_EQUAL(t_f64, t_c8);
  REGISTER_EQUAL(t_f64, t_i64);
  REGISTER_EQUAL(t_f64, t_f64);
  global_dict_add_ffi2("=", equal_table);
}

#pragma endregion equal

#pragma region less

t_ffi less_table[T_MAX][T_MAX] = {};

#define LESS_OP(a, b) (a) < (b)

GEN_BINOP_SPECIALIZATION(less, t_c8, t_c8, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_c8, t_i64, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_c8, t_f64, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_i64, t_c8, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_i64, t_i64, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_i64, t_f64, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_f64, t_c8, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_f64, t_i64, t_i64, LESS_OP);
GEN_BINOP_SPECIALIZATION(less, t_f64, t_f64, t_i64, LESS_OP);

#define REGISTER_less(t1, t2) less_table[TYPE_ENUM(t1)][TYPE_ENUM(t2)] = less_##t1##_##t2;

CONSTRUCTOR void register_less() {
  REGISTER_less(t_c8, t_c8);
  REGISTER_less(t_c8, t_i64);
  REGISTER_less(t_c8, t_f64);
  REGISTER_less(t_i64, t_c8);
  REGISTER_less(t_i64, t_i64);
  REGISTER_less(t_i64, t_f64);
  REGISTER_less(t_f64, t_c8);
  REGISTER_less(t_f64, t_i64);
  REGISTER_less(t_f64, t_f64);
  global_dict_add_ffi2("<", less_table);
}

#pragma endregion less

#pragma endregion binops

#pragma region array_create

DEF_WORD_1_1("index", index) {
  own(shape_t) s = as_shape(x);
  own(array_t) y = array_alloc(T_I64, shape_len(*s), *s);
  DO_MUT_ARRAY(y, t_i64, i, ptr) { (*ptr) = i; }
  return array_inc_ref(y);
}

DEF_WORD("pascal", pascal) {
  POP(x);
  size_t n        = as_size_t(x);
  dim_t  dims[2]  = {n, n};
  own(array_t) y  = array_alloc(T_I64, n * n, shape_create(2, dims));
  t_i64* ptr      = array_mut_data(y);

  DO(i, n) ptr[i] = ptr[i * n]                         = 1;
  DO(i, n) DO(j, n) if (i > 0 && j > 0) ptr[i * n + j] = ptr[i * n + j - 1] + ptr[(i - 1) * n + j];

  PUSH(y);
}

#pragma endregion array_create

#pragma region array_ops

DEF_WORD("reverse", reverse) {
  POP(x);
  own(array_t) y = array_alloc(x->t, x->n, array_shape(x));
  DO(i, x->n) { memcpy(array_mut_data_i(y, i), array_data_i(x, x->n - i - 1), type_sizeof(x->t, 1)); }
  PUSH(y);
}

DEF_WORD("reshape", reshape) {
  POP(x);
  POP(y);
  own(shape_t) s = as_shape(x);
  own(array_t) z = array_alloc(y->t, shape_len(*s), *s);
  size_t ys      = type_sizeof(y->t, y->n);
  DO(i, type_sizeof(y->t, z->n)) { ((char*)array_mut_data(z))[i] = ((char*)array_data(y))[i % ys]; }
  PUSH(z);
}

DEF_WORD_1_1("shape", shape) {
  dim_t d        = x->r;
  own(array_t) y = array_alloc(T_I64, x->r, shape_1d(&d));
  DO_MUT_ARRAY(y, t_i64, i, p) { *p = array_dims(x)[i]; }
  return array_inc_ref(y);
}
DEF_WORD_1_1("len", len) { return array_inc_ref(array_move(array_new_scalar_t_i64(x->n))); }

DEF_WORD("[]", cell) {
  POP(y);
  CHECK(y->r <= 1, "array rank <=1 expected");
  CHECK(y->t == T_I64, "i64 array expected");
  POP(x);
  own(array_t) z = array_get_cell(x, y->n, array_data_t_i64(y));
  PUSH(z);
}

DEF_WORD("cat", cat) {
  POP(x);
  protected(shape_t) s = protect(as_shape(x));
  own(array_t) y       = catenate(stack, *s);
  PUSH(y);
}

#pragma endregion array_ops

#pragma region slash_words

DEF_WORD("\\i", slash_info) {
  printf(VERSION_STRING "\n");
  printf("  %-20s %10ld entries\n", "stack size:", stack_len(stack));
  {
    size_t allocated;
    size_t sz = sizeof(allocated);
    CHECK(mallctl("stats.allocated", &allocated, &sz, NULL, 0) == 0, "failed to query heap size");
    printf("  %-20s %10ld bytes\n", "allocated mem:", allocated);
  }
}

DEF_WORD("\\c", slash_clear) { inter_reset(inter); }
DEF_WORD("\\mem", slash_mem) { malloc_stats_print(NULL, NULL, NULL); }
DEF_WORD("\\s", slash_stack) { DO(i, stack_len(stack)) fprintf(inter->out, "%ld: %pA\n", i, stack_peek(stack, i)); }

#pragma endregion slash_words

#pragma region adverbs

DEF_WORD("fold[]", fold_cell) {
  POP(op);
  POP(r);
  POP(x);

  t_dict_entry e = as_dict_entry(op);

  void __iter(size_t i, array_t * slice) {
    PUSH(slice);
    if (i > 0) inter_dict_entry(inter, e);
  }
  array_for_each_cell(x, as_size_t(r), __iter);
}

DEF_WORD("scan[]", scan_cell) {
  POP(op);
  POP(r);
  POP(x);

  t_dict_entry e    = as_dict_entry(op);
  size_t       rank = as_size_t(r);

  void __iter(size_t i, array_t * slice) {
    if (i > 0) DUP;
    PUSH(slice);
    if (i > 0) inter_dict_entry(inter, e);
  }
  array_for_each_cell(x, rank, __iter);
  own(array_t) result = catenate(stack, shape_prefix(array_shape(x), x->r - rank));
  PUSH(result);
}

DEF_WORD("apply[]", apply_cell) {
  POP(op);
  POP(r);
  POP(x);

  t_dict_entry e    = as_dict_entry(op);
  size_t       rank = as_size_t(r);

  void __iter(size_t i, array_t * slice) {
    PUSH(slice);
    inter_dict_entry(inter, e);
  }
  array_for_each_cell(x, rank, __iter);
  own(array_t) result = catenate(stack, shape_prefix(array_shape(x), x->r - rank));
  PUSH(result);
}

DEF_WORD("pairwise[]", pairwise_cell) {
  POP(op);
  POP(r);
  POP(x);

  t_dict_entry e    = as_dict_entry(op);
  size_t       rank = as_size_t(r);

  void __iter(size_t i, array_t * slice) {
    PUSH(slice);
    if (i > 0) inter_dict_entry(inter, e);
    PUSH(slice);
  }
  array_for_each_cell(x, rank, __iter);
  DROP;
  own(array_t) result = catenate(stack, shape_prefix(array_shape(x), x->r - rank));
  PUSH(result);
}

DEF_WORD("power", power) {
  POP(op);
  POP(n);

  t_dict_entry e = as_dict_entry(op);
  DO(i, as_size_t(n)) { inter_dict_entry(inter, e); }
}

DEF_WORD("trace", trace) {
  POP(op);
  POP(y);

  t_dict_entry e = as_dict_entry(op);
  own(shape_t) s = as_shape(y);

  DO(i, shape_len(*s)) {
    if (i > 0) DUP;
    inter_dict_entry(inter, e);
  }

  own(array_t) result = catenate(stack, *s);
  PUSH(result);
}

#pragma endregion adverbs

#pragma region io

DEF_WORD(".", dot) {
  POP(x);
  fprintf(inter->out, "%pA\n", x);
}

DEF_WORD_1_1("load_text", load_text) {
  CHECK(x->t == T_C8, "c8 array expected");
  CHECK(x->r >= 1, "rank >= 1 expected");
  str_t name       = str_new_len(array_data_t_c8(x), x->n);
  own(char) c_name = str_toc(name);
  own(FILE) file   = fopen(c_name, "r");
  CHECK(file, "failed to open file %s", c_name);
  CHECK(!fseek(file, 0, SEEK_END), "failed to seek file");
  size_t n       = ftell(file);
  own(array_t) y = array_alloc(T_C8, n, shape_1d(&n));
  rewind(file);
  size_t read = fread(array_mut_data(y), 1, n, file);
  CHECK(n == read, "truncated read");
  return array_inc_ref(y);
}

#pragma endregion io