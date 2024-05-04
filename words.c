#include "farr.h"

typedef void (*binop_t)(size_t n, const void* restrict a, const void* restrict b, void* restrict y);

STATUS_T w_binop(stack_t* stack,
                 type_t type_table[T_MAX][T_MAX],
                 binop_t op_table[T_MAX][T_MAX],
                 binop_t a_scalar_table[T_MAX][T_MAX],
                 binop_t b_scalar_table[T_MAX][T_MAX]) {
  own(array_t) b = stack_pop(stack);
  own(array_t) a = stack_pop(stack);

  binop_t op;
  shape_t s;
  size_t n;

  if (is_atom(a)) {
    op = a_scalar_table[a->t][b->t];
    n = b->n;
    s = array_shape(b);
  } else if (is_atom(b)) {
    op = b_scalar_table[a->t][b->t];
    n = a->n;
    s = array_shape(a);
  } else if (a->n == b->n) {
    op = op_table[a->t][b->t];
    n = a->n;
    s = array_shape(a);
  } else {
    return status_errf("array shape doesn't match");
  }

  if (op == NULL) return status_errf("unsupported types");
  array_t* y = array_alloc(type_table[a->t][b->t], n, s);
  op(y->n, array_data(a), array_data(b), array_mut_data(y));
  stack_push(stack, y);
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

#define GEN_BINOP(word, name, op)                                                                             \
  GEN_BINOP_SPECIALIZATIONS(name, t_i64, t_i64, t_i64, op)                                                    \
  GEN_BINOP_SPECIALIZATIONS(name, t_i64, t_f64, t_f64, op)                                                    \
  GEN_BINOP_SPECIALIZATIONS(name, t_f64, t_i64, t_f64, op)                                                    \
  GEN_BINOP_SPECIALIZATIONS(name, t_f64, t_f64, t_f64, op)                                                    \
  binop_t name##_table[T_MAX][T_MAX] =                                                                        \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_t_i64_t_i64, name##_t_i64_t_f64, NULL),                        \
               /* f64 */ TYPE_ROW(NULL, name##_t_f64_t_i64, name##_t_f64_t_f64, NULL),                        \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL));                                                   \
  binop_t name##_a_scalar_table[T_MAX][T_MAX] =                                                               \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_scalar_t_i64_t_i64, name##_scalar_t_i64_t_f64, NULL),          \
               /* f64 */ TYPE_ROW(NULL, name##_scalar_t_f64_t_i64, name##_scalar_t_f64_t_f64, NULL),          \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL));                                                   \
  binop_t name##_b_scalar_table[T_MAX][T_MAX] =                                                               \
      TYPE_ROW(/* c8 */ TYPE_ROW(NULL, NULL, NULL, NULL),                                                     \
               /* i64 */ TYPE_ROW(NULL, name##_t_i64_scalar_t_i64, name##_t_i64_scalar_t_f64, NULL),          \
               /* f64 */ TYPE_ROW(NULL, name##_t_f64_scalar_t_i64, name##_t_f64_scalar_t_f64, NULL),          \
               /* arr */ TYPE_ROW(NULL, NULL, NULL, NULL));                                                   \
  type_t name##_type_table[T_MAX][T_MAX] =                                                                    \
      TYPE_ROW(/* c8 */ TYPE_ROW(T_C8, T_I64, T_F64, T_ARR), /* i64 */ TYPE_ROW(T_C8, T_I64, T_F64, T_ARR),   \
               /* f64 */ TYPE_ROW(T_C8, T_F64, T_F64, T_ARR), /* arr */ TYPE_ROW(T_C8, T_I64, T_F64, T_ARR)); \
  DEF_WORD(word, name) {                                                                                      \
    return w_binop(stack, name##_type_table, name##_table, name##_a_scalar_table, name##_b_scalar_table);     \
  }

#define PLUS_OP(a, b) a + b
#define MUL_OP(a, b) a* b
#define MINUS_OP(a, b) a - b
#define DIV_OP(a, b) a / b

GEN_BINOP("+", plus, PLUS_OP)
GEN_BINOP("*", mul, MUL_OP)
GEN_BINOP("-", minus, MINUS_OP)
GEN_BINOP("/", div, DIV_OP)

DEF_WORD1("shape", shape) { return result_ok(array_new(T_I64, x->r, shape_1d(&x->r), array_dims(x))); }
DEF_WORD1("len", len) { return result_ok(atom_i64(x->n)); }

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