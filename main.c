#include "farr.h"

#include <getopt.h>

int64_t str_parse_i64(const str_t s) {
  own(char) c = str_toc(s);
  return strtol(c, NULL, 10);
}
double str_parse_f64(const str_t s) {
  own(char) c = str_toc(s);
  return strtod(c, NULL);
}

// builtins

typedef void (*binop_t)(size_t n, void* restrict y, const void* restrict a, const void* restrict b);

STATUS_T w_binop(stack_t* stack,
                 type_t type_table[T_MAX][T_MAX],
                 binop_t op_table[T_MAX][T_MAX],
                 binop_t scalar_table[T_MAX][T_MAX]) {
  own(array_t) b = stack_pop(stack);
  own(array_t) a = stack_pop(stack);

  type_t t = type_table[a->t][b->t];
  if (a->n == b->n) {
    array_t* y = array_alloc(t, a->n, array_shape(a));
    op_table[a->t][b->t](a->n, array_mut_data(y), array_data(a), array_data(b));
    stack_push(stack, y);
  } else if (is_atom(a)) {
    array_t* y = array_alloc(t, b->n, array_shape(b));
    scalar_table[a->t][b->t](b->n, array_mut_data(y), array_data(b), array_data(a));
    stack_push(stack, y);
  } else if (is_atom(b)) {
    array_t* y = array_alloc(t, a->n, array_shape(a));
    scalar_table[a->t][b->t](a->n, array_mut_data(y), array_data(a), array_data(b));
    stack_push(stack, y);
  } else {
    assert(false);  // todo: return error
  }

  return status_ok();
}

#define GEN_BINOP_LOOP(name, y_t, a_t, b_t, expr)                                                     \
  void name(size_t n, void* restrict y_ptr, const void* restrict a_ptr, const void* restrict b_ptr) { \
    y_t* restrict y = y_ptr;                                                                          \
    const a_t* restrict a = a_ptr;                                                                    \
    const b_t* restrict b = b_ptr;                                                                    \
    DO(i, n) y[i] = (expr);                                                                           \
  }

#define GEN_BINOP(name, op)                                                                                   \
  GEN_BINOP_LOOP(name##_ai64_ai64, i64, i64, i64, (a[i])op(b[i]))                                             \
  GEN_BINOP_LOOP(name##_ai64_af64, f64, i64, f64, (a[i])op(b[i]))                                             \
  GEN_BINOP_LOOP(name##_af64_ai64, f64, f64, i64, (a[i])op(b[i]))                                             \
  GEN_BINOP_LOOP(name##_af64_af64, f64, f64, f64, (a[i])op(b[i]))                                             \
  GEN_BINOP_LOOP(name##_ai64_i64, i64, i64, i64, (a[i])op(*b))                                                \
  GEN_BINOP_LOOP(name##_ai64_f64, i64, i64, f64, (a[i])op(*b))                                                \
  GEN_BINOP_LOOP(name##_af64_i64, i64, f64, i64, (a[i])op(*b))                                                \
  GEN_BINOP_LOOP(name##_af64_f64, f64, f64, f64, (a[i])op(*b))                                                \
  binop_t name##_table[T_MAX][T_MAX] = TYPE_ROW(/* i64 */ TYPE_ROW(name##_ai64_ai64, name##_ai64_af64),       \
                                                /* f64 */ TYPE_ROW(name##_af64_ai64, name##_af64_af64));      \
  binop_t name##_scalar_table[T_MAX][T_MAX] = TYPE_ROW(/* i64 */ TYPE_ROW(name##_ai64_i64, name##_ai64_f64),  \
                                                       /* f64 */ TYPE_ROW(name##_af64_i64, name##_af64_f64)); \
  type_t name##_type_table[T_MAX][T_MAX] =                                                                    \
      TYPE_ROW(/* i64 */ TYPE_ROW(T_I64, T_F64), /* f64 */ TYPE_ROW(T_F64, T_F64));                           \
  STATUS_T w_##name(stack_t* stack) { return w_binop(stack, name##_type_table, name##_table, name##_scalar_table); }

GEN_BINOP(plus, +)
GEN_BINOP(mul, *)

RESULT_T w_shape(const array_t* x) { return result_ok(array_new(T_I64, x->r, shape_1d(&x->r), array_dims(x))); }

RESULT_T w_len(const array_t* x) { return result_ok(atom_i64(x->n)); }

shape_t create_shape(const array_t* x) {
  assert(x->r <= 1);      // todo: report error
  assert(x->t == T_I64);  // todo: report error
  if (is_atom(x)) return shape_1d(array_data(x));
  return shape_create(x->n, array_data(x));
}

RESULT_T w_zeros(array_t* x) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = 0;
  return result_ok(y);
}

RESULT_T w_ones(array_t* x) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = 1;
  return result_ok(y);
}

RESULT_T w_index(array_t* x) {
  shape_t s = create_shape(x);
  array_t* y = array_alloc(T_I64, shape_len(s), s);
  DO(i, y->n) array_mut_data_i64(y)[i] = i;
  return result_ok(y);
}

STATUS_T concatenate(stack_t* stack, size_t len) {
  if (!len) {
    stack_push(stack, array_alloc(T_I64, 0, shape_1d(&len)));
    return status_ok();
  }
  bool all_common_shape = true;
  const shape_t common_shape = array_shape(stack_peek(stack));
  const type_t t = stack_peek(stack)->t;
  DO(i, len) {
    array_t* e = stack_i(stack, i);
    assert(e->t == t);  // not implemented
    all_common_shape &= shape_eq(common_shape, array_shape(e));
  }
  assert(all_common_shape);  // not implemented
  own(shape_t) new_shape = shape_extend(common_shape, len);
  array_t* a = array_alloc(t, shape_len(*new_shape), *new_shape);
  size_t stride = type_sizeof(t, shape_len(common_shape));
  assert(array_data_sizeof(a) == stride * len);
  DO(i, len) {
    array_t* e = stack_i(stack, len - i - 1);
    assert(array_data_sizeof(e) == stride);
    memcpy(array_mut_data(a) + i * stride, array_data(e), stride);
  }
  DO(i, len) stack_drop(stack);
  stack_push(stack, a);
  return status_ok();
}


// interpreter

typedef struct {
  stack_t* stack;
  size_t arr_level;
  size_t arr_marks[256];
  FILE* out;
} interpreter_t;

interpreter_t* interpreter_new() {
  interpreter_t* i = calloc(1, sizeof(interpreter_t));
  i->stack = stack_new();
  i->out = stdout;
  return i;
}
void interpreter_free(interpreter_t* i) {
  stack_free(i->stack);
  free(i);
}
DEF_CLEANUP(interpreter_t, interpreter_free);

#define CALL1(fn)                         \
  {                                       \
    own(array_t) x = stack_pop(stack);    \
    result_t result = fn(x);              \
    if (result.ok) {                      \
      stack_push(stack, result.either.a); \
      return status_ok();                 \
    } else {                              \
      return status_err(result.either.e); \
    }                                     \
  }

STATUS_T interpreter_word(interpreter_t* inter, const str_t w) {
  stack_t* stack = inter->stack;

  if (str_eqc(w, "+")) {
    return w_plus(stack);
  } else if (str_eqc(w, "*")) {
    return w_mul(stack);
  } else if (str_eqc(w, "shape")) {
    CALL1(w_shape);
  } else if (str_eqc(w, "len")) {
    CALL1(w_len);
  } else if (str_eqc(w, "index")) {
    CALL1(w_index);
  } else if (str_eqc(w, "zeros")) {
    CALL1(w_zeros);
  } else if (str_eqc(w, "ones")) {
    CALL1(w_ones);
  } else if (str_eqc(w, "dup")) {
    stack_push(stack, array_inc_ref(stack_peek(stack)));
  } else if (str_eqc(w, "drop")) {
    stack_drop(stack);
  } else if (str_eqc(w, ".")) {
    own(array_t) x = stack_pop(stack);
    fprintf(inter->out, "%pA\n", x);
  } else if (str_eqc(w, "swap")) {
    array_t* a = stack_pop(stack);
    array_t* b = stack_pop(stack);
    stack_push(stack, a);
    stack_push(stack, b);
  } else if (str_eqc(w, "exit")) {
    fprintf(inter->out, "bye\n");
    exit(0);
  } else {
    return status_errf("unknown word '%pS'", &w);
  }

  return status_ok();
}

#undef CALL1

STATUS_T interpreter_token(interpreter_t* inter, token_t t) {
  switch (t.tok) {
    case TOK_EOF: return status_ok();
    case TOK_ERR: return status_errf("unexpected token: '%pS'", &t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = inter->stack->l;
      return status_ok();
    }
    case TOK_ARR_CLOSE: {
      assert(inter->arr_level); // todo: report error
      size_t mark = inter->arr_marks[--inter->arr_level];
      assert(inter->stack->l >= mark); // todo: report error
      return concatenate(inter->stack, inter->stack->l - mark);
    }
    case TOK_WORD: return interpreter_word(inter, t.text);
    case TOK_I64: {
      stack_push(inter->stack, atom_i64(str_parse_i64(t.text)));
      return status_ok();
    }
    case TOK_F64: {
      stack_push(inter->stack, atom_f64(str_parse_f64(t.text)));
      return status_ok();
    }
  }
  UNREACHABLE;
}

STATUS_T interpreter_line(interpreter_t* inter, const char* s) {
  for (;;) {
    token_t t = next_token(&s);
    if (t.tok == TOK_EOF) return status_ok();
    R_ERR(interpreter_token(inter, t));
  }
  return status_ok();
}

// repl

STATUS_T repl() {
  size_t input_size = 0;
  own(char) input = NULL;

  own(interpreter_t) inter = interpreter_new();

  while (true) {
    if (inter->arr_level) {
      printf("%ld> ", inter->arr_level);
    } else {
      stack_print(inter->stack);
      printf(" > ");
    }

    if (getline(&input, &input_size, stdin) <= 0) return status_ok();
    status_t result = interpreter_line(inter, input);
    if (status_is_err(result)) {
      str_t msg = status_msg(result);
      fprintf(stderr, "ERROR: %pS\n", &msg);
    }
  }
}

// test runner

STATUS_T test(const char* fname, bool v) {
  own(FILE) file = fopen(fname, "r");
  if (!file) return status_errf("test: can't open file: %s", fname);

  size_t len = 0;
  own(char) line = NULL;

  own(interpreter_t) inter = interpreter_new();

  size_t read;
  size_t line_no = 0;

  own(char) out = NULL;
  size_t out_size;
  char* rest_out = NULL;
  size_t in_line_no = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    line_no++;
    if (read == 0 || *line == '#') continue;
    if (*line == '>') {
      if (v) fprintf(stderr, "%s", line);
      if (rest_out && *rest_out) fprintf(stderr, "ERROR %s:%ld : umatched output: '%s'\n", fname, in_line_no, rest_out);
      if (out) free(out);
      in_line_no = line_no;
      FILE* fout = open_memstream(&out, &out_size);
      inter->out = fout;
      status_t result = interpreter_line(inter, line + 1);
      inter->out = stdout;
      fclose(fout);

      if (status_is_err(result)) {
        str_t msg = status_msg(result);
        fprintf(stderr, "ERROR %s:%ld : '%pS'\n", fname, line_no, &msg);
        continue;
      }
      rest_out = out;
      continue;
    }

    if (memcmp(line, rest_out, read)) {
      fprintf(stderr, "ERROR %s:%ld : mismatched output, expected: '%s' actual: '%s' \n", fname, in_line_no, line,
              rest_out);
    }
    rest_out += read;
  }

  if (rest_out && *rest_out) fprintf(stderr, "ERROR %s:%ld : umatched output: '%s'\n", fname, in_line_no, rest_out);

  return status_ok();
}

STATUS_T eval(const char* stmt) {
  own(interpreter_t) inter = interpreter_new();
  return interpreter_line(inter, stmt);
}

// main

int main(int argc, char* argv[]) {
  char* t = NULL;
  bool v = false;
  char* e = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "vht:e:")) != -1) {
    switch (opt) {
      case 't': t = optarg; break;
      case 'v': v = true; break;
      case 'e': e = optarg; break;
      case 'h':
      default:
        fprintf(stderr, "Usage: %s | %s -t test_file [-v] | %s -e stmt | %s -h \n", argv[0], argv[0], argv[0], argv[0]);
        exit(1);
    }
  }

  status_t s;
  if (t) s = test(t, v);
  else if (e) s = eval(e);
  else s = repl();
  if (status_is_ok(s)) return 0;
  status_print_error(s);
  return 1;
}