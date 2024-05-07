#include <math.h>

#include "niko.h"

#define GEN_REGISTER_SPEC(name, t) \
  CONSTRUCTOR(500) void register_##name##_##t() { name##_table[TYPE_ENUM(t)] = name##_##t; }

INLINE STATUS_T thread1(interpreter_t* inter, stack_t* stack, const array_t* x, t_ffi ffi_table[T_MAX]) {
  assert(x->t == T_ARR);
  array_t* out = array_alloc_as(x);
  DO(i, x->n) {
    stack_push(stack, ((const array_t**)array_data(x))[i]);
    t_ffi ffi = ffi_table[stack_peek(stack)->t];
    assert(ffi);
    R_IF_ERR(ffi(inter, stack));
    ((array_t**)array_mut_data(out))[i] = stack_pop(stack);
  }
  stack_push(stack, out);
  return status_ok();
}

#define GEN_THREAD1(name, ffi)            \
  DEF_WORD_HANDLER(name##_t_arr) {        \
    own(array_t) x = stack_pop(stack);    \
    return thread1(inter, stack, x, ffi); \
  }                                       \
  GEN_REGISTER_SPEC(name, t_arr)

// neg

t_ffi neg_table[T_MAX];

#define GEN_NEG(t)                                                           \
  DEF_WORD_HANDLER(neg_##t) {                                                \
    own(array_t) x = stack_pop(stack);                                       \
    array_t* out = array_alloc_as(x);                                        \
    DO(i, x->n)((t*)array_mut_data(out))[i] = -((const t*)array_data(x))[i]; \
    stack_push(stack, out);                                                  \
    R_OK;                                                                    \
  }                                                                          \
  GEN_REGISTER_SPEC(neg, t)

GEN_NEG(t_i64);
GEN_NEG(t_f64);

GEN_THREAD1(neg, neg_table)

#define UPDATE_NEG_TABLE(t) neg_table[TYPE_ENUM(t)] = neg_##t;

CONSTRUCTOR(1000) void reg_neg() {
  array_t* a = array_new_1d(T_FFI, T_MAX, neg_table);
  entry_vector_add(&global_dict, (dict_entry_t){string_newf("neg"), a});
}

// sqrt, sin, et. al.

#define GEN_FLOAT1_SPEC(name, t, op)                                                \
  DEF_WORD_HANDLER(name##_##t) {                                                    \
    own(array_t) x = stack_pop(stack);                                              \
    array_t* out = array_alloc(T_F64, x->n, array_shape(x));                        \
    DO(i, x->n)((t_f64*)array_mut_data(out))[i] = op(((const t*)array_data(x))[i]); \
    stack_push(stack, out);                                                         \
    R_OK;                                                                           \
  }                                                                                 \
  CONSTRUCTOR(500) void register_##name##_##t() { name##_table[TYPE_ENUM(t)] = name##_##t; }

#define GEN_FLOAT(name, op) \
t_ffi name##_table[T_MAX]; \
GEN_FLOAT1_SPEC(name, t_f64, op) \
GEN_FLOAT1_SPEC(name, t_i64, op) \
GEN_THREAD1(name, name##_table) \
CONSTRUCTOR(1000) void reg_##name() { \
  array_t* a = array_new_1d(T_FFI, T_MAX, name##_table); \
  entry_vector_add(&global_dict, (dict_entry_t){string_newf(#name), a}); \
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

// binops

typedef void (*binop_t)(size_t n, const void* restrict x, const void* restrict y, void* restrict out);

STATUS_T w_binop(stack_t* stack,
                 type_t type_table[T_MAX][T_MAX],
                 binop_t op_table[T_MAX][T_MAX],
                 binop_t x_scalar_table[T_MAX][T_MAX],
                 binop_t y_scalar_table[T_MAX][T_MAX]) {
  own(array_t) y = stack_pop(stack);
  own(array_t) x = stack_pop(stack);

  binop_t op;
  shape_t s;
  size_t n;

  if (is_atom(x)) {
    op = x_scalar_table[x->t][y->t];
    n = y->n;
    s = array_shape(y);
  } else if (is_atom(y)) {
    op = y_scalar_table[x->t][y->t];
    n = x->n;
    s = array_shape(x);
  } else if (x->n == y->n) {
    op = op_table[x->t][y->t];
    n = x->n;
    s = array_shape(x);
  } else {
    return status_errf("array shape doesn't match");
  }

  if (op == NULL) return status_errf("unsupported types");
  array_t* out = array_alloc(type_table[x->t][y->t], n, s);
  op(out->n, array_data(x), array_data(y), array_mut_data(out));
  stack_push(stack, out);
  R_OK;
}

#define GEN_BINOP_SPECIALIZATION(name, a_t, b_t, y_t, expr)                                           \
  void name(size_t n, const void* restrict a_ptr, const void* restrict b_ptr, void* restrict y_ptr) { \
    y_t* restrict y = y_ptr;                                                                          \
    const a_t* restrict a = a_ptr;                                                                    \
    const b_t* restrict b = b_ptr;                                                                    \
    DO(i, n) y[i] = (expr);                                                                           \
  }

#define GEN_BINOP_SPECIALIZATIONS(name, a_t, b_t, y_t, op)                                 \
  GEN_BINOP_SPECIALIZATION(name##_##a_t##_##b_t, a_t, b_t, y_t, op((a[i]), (b[i])))        \
  GEN_BINOP_SPECIALIZATION(name##_scalar_##a_t##_##b_t, a_t, b_t, y_t, op((a[0]), (b[i]))) \
  GEN_BINOP_SPECIALIZATION(name##_##a_t##_scalar_##b_t, a_t, b_t, y_t, op((a[i]), (b[0])))

#define GEN_BINOP(word, name, op)                                                                                   \
  GEN_BINOP_SPECIALIZATIONS(name, t_i64, t_i64, t_i64, op)                                                          \
  GEN_BINOP_SPECIALIZATIONS(name, t_i64, t_f64, t_f64, op)                                                          \
  GEN_BINOP_SPECIALIZATIONS(name, t_f64, t_i64, t_f64, op)                                                          \
  GEN_BINOP_SPECIALIZATIONS(name, t_f64, t_f64, t_f64, op)                                                          \
  binop_t name##_table[T_MAX][T_MAX] =                                                                              \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_t_i64_t_i64, name##_t_i64_t_f64, NULL, NULL),                        \
               /* f64 */ TYPE_ROW(NULL, name##_t_f64_t_i64, name##_t_f64_t_f64, NULL, NULL),                        \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL), /* ffi */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL)); \
  binop_t name##_a_scalar_table[T_MAX][T_MAX] =                                                                     \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_scalar_t_i64_t_i64, name##_scalar_t_i64_t_f64, NULL, NULL),          \
               /* f64 */ TYPE_ROW(NULL, name##_scalar_t_f64_t_i64, name##_scalar_t_f64_t_f64, NULL, NULL),          \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL), /* ffi */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL)); \
  binop_t name##_b_scalar_table[T_MAX][T_MAX] =                                                                     \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_t_i64_scalar_t_i64, name##_t_i64_scalar_t_f64, NULL, NULL),          \
               /* f64 */ TYPE_ROW(NULL, name##_t_f64_scalar_t_i64, name##_t_f64_scalar_t_f64, NULL, NULL),          \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL), /* ffi */ TYPE_ROW(NULL, NULL, NULL, NULL, NULL)); \
  type_t name##_type_table[T_MAX][T_MAX] =                                                                          \
      TYPE_ROW(/* c8 */ TYPE_ROW_ID, /* i64 */ TYPE_ROW(T_C8, T_I64, T_F64, T_ARR, T_FFI),                          \
               /* f64 */ TYPE_ROW(T_C8, T_F64, T_F64, T_ARR, T_FFI),                                                \
               /* arr */ TYPE_ROW(T_C8, T_I64, T_F64, T_ARR, T_FFI), /* ffi */ TYPE_ROW_ID);                        \
  DEF_WORD(word, name) {                                                                                            \
    return w_binop(stack, name##_type_table, name##_table, name##_a_scalar_table, name##_b_scalar_table);           \
  }

#define PLUS_OP(a, b) (a) + (b)
#define MUL_OP(a, b) (a) * (b)
#define MINUS_OP(a, b) (a) - (b)
#define DIV_OP(a, b) (a) / (b)

GEN_BINOP("+", plus, PLUS_OP)
GEN_BINOP("*", mul, MUL_OP)
GEN_BINOP("-", minus, MINUS_OP)
GEN_BINOP("/", div, DIV_OP)

DEF_WORD1("shape", shape) { return result_ok(array_new(T_I64, x->r, shape_1d(&x->r), array_dims(x))); }
DEF_WORD1("len", len) { return result_ok(atom_t_i64(x->n)); }

shape_t create_shape(const array_t* x) {
  assert(x->r <= 1);      // todo: report error
  assert(x->t == T_I64);  // todo: report error
  if (is_atom(x)) return shape_1d(array_data(x));
  return shape_create(x->n, array_data(x));
}

DEF_WORD1("zeros", zeros) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = 0;
  return result_ok(y);
}

DEF_WORD1("ones", ones) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = 1;
  return result_ok(y);
}

DEF_WORD1("index", index) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = i;
  return result_ok(y);
}

DEF_WORD("dup", dup) {
  stack_push(stack, array_inc_ref(stack_peek(stack)));
  R_OK;
}

DEF_WORD("swap", swap) {
  array_t* a = stack_pop(stack);
  array_t* b = stack_pop(stack);
  stack_push(stack, a);
  stack_push(stack, b);
  R_OK;
}

DEF_WORD("drop", drop) {
  stack_drop(stack);
  R_OK;
}

DEF_WORD(".", dot) {
  own(array_t) x = stack_pop(stack);
  fprintf(inter->out, "%pA\n", x);
  R_OK;
}

DEF_WORD("exit", exit) {
  fprintf(inter->out, "bye\n");
  exit(0);
}