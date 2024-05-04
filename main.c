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

entry_vector_t global_dict;

STATUS_T concatenate(stack_t* stack, size_t len) {
  if (!len) {
    stack_push(stack, array_alloc(T_I64, 0, shape_1d(&len)));
    return status_ok();
  }
  bool all_common = true;
  const shape_t common_shape = array_shape(stack_peek(stack));
  const type_t t = stack_peek(stack)->t;
  DO(i, len) {
    array_t* e = stack_i(stack, i);
    all_common &= e->t == t;
    all_common &= shape_eq(common_shape, array_shape(e));
  }

  array_t* a;
  if (!all_common) {
    a = array_alloc(T_ARR, len, shape_1d(&len));
    DO(i, len) { ((array_t**)array_mut_data(a))[i] = array_inc_ref(stack_i(stack, len - i - 1)); }
  } else {
    own(shape_t) new_shape = shape_extend(common_shape, len);
    a = array_alloc(t, shape_len(*new_shape), *new_shape);
    size_t stride = type_sizeof(t, shape_len(common_shape));
    assert(array_data_sizeof(a) == stride * len);
    DO(i, len) {
      array_t* e = stack_i(stack, len - i - 1);
      assert(array_data_sizeof(e) == stride);
      memcpy(array_mut_data(a) + i * stride, array_data(e), stride);
    }
  }
  DO(i, len) stack_drop(stack);
  stack_push(stack, a);
  return status_ok();
}

// interpreter

typedef struct {
  entry_vector_t dict;
  stack_t* stack;
  size_t arr_level;
  size_t arr_marks[256];
  FILE* out;
} interpreter_t;

interpreter_t* interpreter_new() {
  interpreter_t* inter = calloc(1, sizeof(interpreter_t));
  inter->stack = stack_new();
  inter->out = stdout;
  DO(i, global_dict.l) entry_vector_add(&inter->dict, global_dict.d[i]);
  return inter;
}
void interpreter_free(interpreter_t* i) {
  stack_free(i->stack);
  free(i);
}
DEF_CLEANUP(interpreter_t, interpreter_free);

STATUS_T interpreter_word(interpreter_t* inter, const str_t w) {
  stack_t* stack = inter->stack;

  DO(i, inter->dict.l) if (str_eq(w, string_as_str(inter->dict.d[i].n))) { return inter->dict.d[i].w(stack); }

  if (str_eqc(w, ".")) {
    own(array_t) x = stack_pop(stack);
    fprintf(inter->out, "%pA\n", x);
  } else if (str_eqc(w, "exit")) {
    fprintf(inter->out, "bye\n");
    exit(0);
  } else {
    return status_errf("unknown word '%pS'", &w);
  }

  return status_ok();
}

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
      assert(inter->arr_level);  // todo: report error
      size_t mark = inter->arr_marks[--inter->arr_level];
      assert(inter->stack->l >= mark);  // todo: report error
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
    case TOK_STR: {
      size_t l = t.text.l - 2;
      stack_push(inter->stack, array_new(T_C8, l, shape_1d(&l), t.text.p + 1));
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
      stack_clear(inter->stack);
      FILE* fout = open_memstream(&out, &out_size);
      inter->out = fout;
      status_t result = interpreter_line(inter, line + 1);
      inter->out = stdout;
      fclose(fout);

      if (status_is_err(result)) {
        free(out);
        str_t msg = status_msg(result);
        out_size = asprintf(&out, "ERROR: %pS\n", &msg);
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