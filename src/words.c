#include <jemalloc/jemalloc.h>
#include <math.h>

#include "niko.h"

// common utilities

STATUS_T as_size_t(array_t* a, size_t* out) {
  STATUS_CHECK(a->r == 0, "scalar expected");
  STATUS_CHECK(a->t == T_I64, "int scalar expected");
  i64 i = *array_data_t_i64(a);
  STATUS_CHECK(i >= 0, "non-negative scalar expected");
  *out = i;
  STATUS_OK;
}

#pragma region stack

#define DUP(stack) stack_push(stack, stack_peek(stack, 0))

DEF_WORD("dup", dup) {
  DUP(stack);
  STATUS_OK;
}

DEF_WORD("swap", swap) {
  own(array_t) a = stack_pop(stack);
  own(array_t) b = stack_pop(stack);
  stack_push(stack, a);
  stack_push(stack, b);
  STATUS_OK;
}

DEF_WORD("drop", drop) {
  stack_drop(stack);
  STATUS_OK;
}

DEF_WORD("nip", nip) {
  own(array_t) a = stack_pop(stack);
  stack_drop(stack);
  stack_push(stack, a);
  STATUS_OK;
}

DEF_WORD("over", over) {
  own(array_t) a = stack_i(stack, 1);
  stack_push(stack, a);
  STATUS_OK;
}

DEF_WORD("rot", rot) {
  own(array_t) a = stack_pop(stack);
  own(array_t) b = stack_pop(stack);
  own(array_t) c = stack_pop(stack);
  stack_push(stack, b);
  stack_push(stack, a);
  stack_push(stack, c);
  STATUS_OK;
}

DEF_WORD("tuck", tuck) {
  own(array_t) a = stack_pop(stack);
  own(array_t) b = stack_pop(stack);
  stack_push(stack, a);
  stack_push(stack, b);
  stack_push(stack, a);
  STATUS_OK;
}

DEF_WORD("pick", pick) {
  STATUS_CHECK(stack_len(stack) > 0, "stack underflow: 1 value expected");
  own(array_t) x = stack_pop(stack);
  size_t n = 0;
  STATUS_UNWRAP(as_size_t(x, &n));
  STATUS_CHECK(stack_len(stack) > n, "stack underflow: index %ld >= stack size %ld", n, stack_len(stack));

  stack_push(stack, array_move(stack_i(stack, n)));
  STATUS_OK;
}

#pragma endregion stack

INLINE STATUS_T thread1(inter_t* inter, stack_t* stack, const array_t* x, t_ffi ffi_table[T_MAX]) {
  assert(x->t == T_ARR);
  own(array_t) out = array_alloc_as(x);

  array_t* const* src = array_data_t_arr(x);
  array_t** dst = array_mut_data_t_arr(out);
  DO(i, x->n) {
    stack_push(stack, src[i]);
    t_ffi ffi = ffi_table[src[i]->t];
    assert(ffi);
    STATUS_UNWRAP(ffi(inter, stack));
    dst[i] = stack_pop(stack);
  }
  stack_push(stack, out);
  return status_ok();
}

#define GEN_THREAD1(name, ffi)            \
  DEF_WORD_HANDLER(name##_t_arr) {        \
    own(array_t) x = stack_pop(stack);    \
    return thread1(inter, stack, x, ffi); \
  }

#pragma region math

// neg

t_ffi neg_table[T_MAX];

#define GEN_NEG(t)                                                           \
  DEF_WORD_HANDLER_1_1(neg_##t) {                                            \
    own(array_t) out = array_alloc_as(x);                                    \
    DO(i, x->n)((t*)array_mut_data(out))[i] = -((const t*)array_data(x))[i]; \
    return result_ok(out);                                                   \
  }

GEN_NEG(t_i64);
GEN_NEG(t_f64);
GEN_THREAD1(neg, neg_table)

CONSTRUCTOR void reg_neg() {
  neg_table[T_I64] = neg_t_i64;
  neg_table[T_F64] = neg_t_f64;
  neg_table[T_ARR] = neg_t_arr;
  own(array_t) a = array_new_1d(T_FFI, T_MAX, neg_table);
  global_dict_add_new(str_from_c("neg"), a);
}

// abs

t_ffi abs_table[T_MAX];

#define GEN_ABS(t, op)                                                          \
  DEF_WORD_HANDLER_1_1(abs_##t) {                                               \
    own(array_t) out = array_alloc_as(x);                                       \
    DO(i, x->n)((t*)array_mut_data(out))[i] = op(((const t*)array_data(x))[i]); \
    return result_ok(out);                                                      \
  }

GEN_ABS(t_i64, labs);
GEN_ABS(t_f64, fabs);
GEN_THREAD1(abs, abs_table)

CONSTRUCTOR void reg_abs() {
  abs_table[T_I64] = abs_t_i64;
  abs_table[T_F64] = abs_t_f64;
  abs_table[T_ARR] = abs_t_arr;
  own(array_t) a = array_new_1d(T_FFI, T_MAX, abs_table);
  global_dict_add_new(str_from_c("abs"), a);
}

// sqrt, sin, et. al.

#define GEN_FLOAT1_SPEC(name, t, op)                                                \
  DEF_WORD_HANDLER(name##_##t) {                                                    \
    own(array_t) x = stack_pop(stack);                                              \
    own(array_t) out = array_alloc(T_F64, x->n, array_shape(x));                    \
    DO(i, x->n)((t_f64*)array_mut_data(out))[i] = op(((const t*)array_data(x))[i]); \
    stack_push(stack, out);                                                         \
    STATUS_OK;                                                                      \
  }

#define GEN_FLOAT(name, op)                                    \
  t_ffi name##_table[T_MAX];                                   \
  GEN_FLOAT1_SPEC(name, t_f64, op)                             \
  GEN_FLOAT1_SPEC(name, t_i64, op)                             \
  GEN_THREAD1(name, name##_table)                              \
  CONSTRUCTOR void reg_##name() {                              \
    name##_table[T_I64] = name##_t_i64;                        \
    name##_table[T_F64] = name##_t_f64;                        \
    name##_table[T_ARR] = name##_t_arr;                        \
    own(array_t) a = array_new_1d(T_FFI, T_MAX, name##_table); \
    global_dict_add_new(str_from_c(#name), a);                 \
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
GEN_FLOAT(sin, sin)
GEN_FLOAT(sinh, sinh)
GEN_FLOAT(sqrt, sqrt)
GEN_FLOAT(tan, tan)
GEN_FLOAT(tanh, tanh)

#pragma endregion math

#pragma region binops

typedef void (*binop_kernel_t)(const void* restrict x,
                               size_t x_n,
                               const void* restrict y,
                               size_t y_n,
                               void* restrict out,
                               size_t out_n);

STATUS_T w_binop(stack_t* stack, type_t t, binop_kernel_t kernel) {
  shape_t ys = array_shape(stack_peek(stack, 0));
  shape_t xs = array_shape(stack_peek(stack, 1));
  STATUS_CHECK(shapes_are_compatible(xs, ys), "array shapes are incompatible: %pH vs %pH", &xs, &ys);

  own(array_t) y = stack_pop(stack);
  own(array_t) x = stack_pop(stack);

  own(array_t) out = array_alloc_shape(t, shape_max(xs, ys));
  kernel(array_data(x), x->n, array_data(y), y->n, array_mut_data(out), out->n);
  stack_push(stack, out);
  STATUS_OK;
}

#define GEN_BINOP_KERNEL(name, a_t, b_t, y_t, op)                                                                 \
  void name(const void* restrict a_ptr, size_t a_n, const void* restrict b_ptr, size_t b_n, void* restrict y_ptr, \
            size_t y_n) {                                                                                         \
    y_t* restrict y = y_ptr;                                                                                      \
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

#define GEN_BINOP(word, name, op)                                       \
  GEN_BINOP_SPECIALIZATION(name, t_i64, t_i64, t_i64, op)               \
  GEN_BINOP_SPECIALIZATION(name, t_i64, t_f64, t_f64, op)               \
  GEN_BINOP_SPECIALIZATION(name, t_f64, t_i64, t_f64, op)               \
  GEN_BINOP_SPECIALIZATION(name, t_f64, t_f64, t_f64, op)               \
  t_ffi name##_table[T_MAX][T_MAX] = {};                                \
  CONSTRUCTOR void __register_w_##name() {                              \
    name##_table[T_I64][T_I64] = name##_t_i64_t_i64;                    \
    name##_table[T_F64][T_I64] = name##_t_f64_t_i64;                    \
    name##_table[T_I64][T_F64] = name##_t_i64_t_f64;                    \
    name##_table[T_F64][T_F64] = name##_t_f64_t_f64;                    \
    size_t dims[2] = {T_MAX, T_MAX};                                    \
    shape_t sh = (shape_t){2, dims};                                    \
    own(array_t) a = array_new(T_FFI, T_MAX * T_MAX, sh, name##_table); \
    global_dict_add_new(str_from_c(word), a);                           \
  }

#define PLUS_OP(a, b) (a) + (b)
#define MUL_OP(a, b) (a) * (b)
#define MINUS_OP(a, b) (a) - (b)
#define DIV_OP(a, b) (a) / (b)

GEN_BINOP("+", plus, PLUS_OP)
GEN_BINOP("*", mul, MUL_OP)
GEN_BINOP("-", minus, MINUS_OP)
GEN_BINOP("/", divide, DIV_OP)

#pragma endregion binops

DEF_WORD_1_1("shape", shape) {
  dim_t d = x->r;
  own(array_t) y = array_alloc(T_I64, x->r, shape_1d(&d));
  DO_MUT_ARRAY(y, t_i64, i, p) { *p = array_dims(x)[i]; }
  return result_ok(y);
}
DEF_WORD_1_1("len", len) { return result_ok(array_move(array_new_scalar_t_i64(x->n))); }

// creating arrays

shape_t create_shape(const array_t* x) {
  assert(x->r <= 1);      // todo: report error
  assert(x->t == T_I64);  // todo: report error
  if (array_is_scalar(x)) return shape_1d(array_data(x));
  return shape_create(x->n, array_data(x));
}

// INLINE void fill_simd(size_t n, vmax_i64* restrict SIMD_ALIGNED out, vmax_i64 x) {
//    DO(i, n) out[i] = x;
//  }

DEF_WORD_1_1("index", index) {
  shape_t s = create_shape(x);
  own(array_t) y = array_alloc(T_I64, shape_len(s), s);
  DO_MUT_ARRAY(y, t_i64, i, ptr)(*ptr) = i;
  return result_ok(y);
}

DEF_WORD("reshape", reshape) {
  STATUS_CHECK(stack_len(stack) > 1, "stack underflow: 2 values expected");
  own(array_t) x = stack_pop(stack);
  own(array_t) y = stack_pop(stack);
  shape_t s = create_shape(x);
  own(array_t) z = array_alloc(y->t, shape_len(s), s);
  size_t ys = type_sizeof(y->t, y->n);
  DO(i, type_sizeof(y->t, z->n)) { ((char*)array_mut_data(z))[i] = ((char*)array_data(y))[i % ys]; }
  stack_push(stack, z);
  STATUS_OK;
}

DEF_WORD("pascal", pascal) {
  own(array_t) x = stack_pop(stack);
  size_t n = 0;
  STATUS_UNWRAP(as_size_t(x, &n));

  dim_t dims[2] = {n, n};
  own(array_t) y = array_alloc(T_I64, n * n, shape_create(2, dims));
  t_i64* ptr = array_mut_data(y);

  DO(i, n) ptr[i] = ptr[i * n] = 1;
  DO(i, n) DO(j, n) if (i > 0 && j > 0) ptr[i * n + j] = ptr[i * n + j - 1] + ptr[(i - 1) * n + j];

  stack_push(stack, y);
  STATUS_OK;
}

DEF_WORD("exit", exit) {
  fprintf(inter->out, "bye\n");
  exit(0);
}

// repl

DEF_WORD("\\c", slash_clear) {
  stack_clear(stack);
  inter->arr_level = 0;
  inter->mode = MODE_INTERPRET;
  STATUS_OK;
}

DEF_WORD("\\i", slash_info) {
  printf(VERSION_STRING "\n");
  printf("  %-20s %10ld entries\n", "stack size:", stack_len(stack));
  {
    size_t allocated;
    size_t sz = sizeof(allocated);
    STATUS_CHECK(mallctl("stats.allocated", &allocated, &sz, NULL, 0) == 0, "failed to query heap size");
    printf("  %-20s %10ld bytes\n", "allocated mem:", allocated);
  }
  STATUS_OK;
}

DEF_WORD("\\mem", slash_mem) {
  malloc_stats_print(NULL, NULL, NULL);
  STATUS_OK;
}

DEF_WORD("\\s", slash_stack) {
  DO(i, stack_len(stack)) fprintf(inter->out, "%ld: %pA\n", i, stack_peek(stack, i));
  STATUS_OK;
}

// fold

DEF_WORD("fold_rank", fold_rank) {
  STATUS_CHECK(stack_len(stack) > 2, "stack underflow: 3 values expected");
  own(array_t) op = stack_pop(stack);
  STATUS_CHECK(op->t == T_DICT_ENTRY, "fold: dict entry expected");
  dict_entry_t* e = *array_data_t_dict_entry(op);

  own(array_t) r = stack_pop(stack);
  STATUS_CHECK(r->r == 0, "scalar expected");
  STATUS_CHECK(r->t == T_I64, "int scalar expected");

  own(array_t) x = stack_pop(stack);

  size_t rank = 0;
  STATUS_UNWRAP(as_size_t(r, &rank));

  shape_t cell = shape_cell(array_shape(x), rank);
  size_t l = shape_len(cell);
  size_t stride = type_sizeof(x->t, l);

  const void* ptr = array_data(x);
  DO(i, x->n / l) {
    own(array_t) y = array_new_slice(x, l, cell, ptr + stride * i);
    stack_push(stack, y);
    if (i > 0) STATUS_UNWRAP(inter_dict_entry(inter, e));
  }

  STATUS_OK;
}

DEF_WORD("+'fold", plus_fold) { NOT_IMPLEMENTED; }

// scan

DEF_WORD("scan_rank", scan_rank) {
  STATUS_CHECK(stack_len(stack) > 2, "stack underflow: 3 values expected");
  own(array_t) op = stack_pop(stack);
  STATUS_CHECK(op->t == T_DICT_ENTRY, "fold: dict entry expected");
  dict_entry_t* e = *array_data_t_dict_entry(op);

  own(array_t) r = stack_pop(stack);
  STATUS_CHECK(r->r == 0, "scalar expected");
  STATUS_CHECK(r->t == T_I64, "int scalar expected");

  own(array_t) x = stack_pop(stack);

  size_t rank = 0;
  STATUS_UNWRAP(as_size_t(r, &rank));

  shape_t cell = shape_cell(array_shape(x), rank);
  size_t l = shape_len(cell);
  size_t stride = type_sizeof(x->t, l);

  const void* ptr = array_data(x);
  DO(i, x->n / l) {
    if (i > 0) DUP(stack);
    own(array_t) y = array_new_slice(x, l, cell, ptr + stride * i);
    stack_push(stack, y);
    if (i > 0) STATUS_UNWRAP(inter_dict_entry(inter, e));
  }

  own(array_t) result = RESULT_UNWRAP(concatenate(stack, array_shape(x)));
  stack_push(stack, result);
  STATUS_OK;
}

// io

DEF_WORD(".", dot) {
  STATUS_CHECK(!stack_is_empty(stack), "stack underflow: 1 value expected");
  own(array_t) x = stack_pop(stack);
  fprintf(inter->out, "%pA\n", x);
  STATUS_OK;
}

DEF_WORD_1_1("load_text", load_text) {
  RESULT_CHECK(x->t == T_C8, "c8 array expected");
  RESULT_CHECK(x->r >= 1, "rank >= 1 expected");
  str_t name = str_new_len(array_data_t_c8(x), x->n);
  own(char) c_name = str_toc(name);
  own(FILE) file = fopen(c_name, "r");
  RESULT_CHECK(file, "failed to open file %s", c_name);
  RESULT_CHECK(!fseek(file, 0, SEEK_END), "failed to seek file");
  size_t n = ftell(file);
  own(array_t) y = array_alloc(T_C8, n, shape_1d(&n));
  rewind(file);
  size_t read = fread(array_mut_data(y), 1, n, file);
  RESULT_CHECK(n == read, "truncated read");
  RESULT_OK(y);
}