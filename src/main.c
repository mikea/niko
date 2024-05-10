#include <getopt.h>
#include <unistd.h>

#include "niko.h"

entry_vector_t global_dict;

DESTRUCTOR void global_dict_free() {
  DO(i, global_dict.l) string_free(global_dict.d[i].k);
  free(global_dict.d);
}

STATUS_T concatenate(stack_t* stack, size_t len) {
  if (!len) {
    dim_t d = len;
    stack_push(stack, array_alloc(T_I64, 0, shape_1d(&d)));
    STATUS_OK;
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
    dim_t d = len;
    a = array_alloc(T_ARR, len, shape_1d(&d));
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
  STATUS_OK;
}

// interpreter

interpreter_t* interpreter_new() {
  interpreter_t* inter = calloc(1, sizeof(interpreter_t));
  inter->stack = stack_new();
  inter->out = stdout;
  DO(i, global_dict.l) entry_vector_add(&inter->dict, global_dict.d[i]);
  return inter;
}
void interpreter_free(interpreter_t* inter) {
  stack_free(inter->stack);
  // todo: we didn't copy the stirng in _new, which we should
  // DO(i, inter->dict.l) string_free(inter->dict.d[i].n);
  free(inter->dict.d);
  free(inter);
}
DEF_CLEANUP(interpreter_t, interpreter_free);

STATUS_T interpreter_word(interpreter_t* inter, array_t* a) {
  switch (a->t) {
    case T_FFI: {
      t_ffi f;

      switch (a->r) {
        case 0: {
          f = *(t_ffi*)array_data(a);
          STATUS_CHECK(f, "not implemented");
          return f(inter, inter->stack);
        }
        case 1: {
          STATUS_CHECK(stack_len(inter->stack) >= 1, "stack underflow: 1 value expected");
          array_t* x = stack_peek(inter->stack);
          f = ((t_ffi*)array_data(a))[x->t];
          STATUS_CHECK(f, "%pT is not supported", &x->t);
          return f(inter, inter->stack);
        }
        case 2: NOT_IMPLEMENTED
        default: return status_errf("unexpected ffi rank: %ld", a->r);
      }
    }
    default: return status_errf("unexpected type: %d", a->t);
  }
}

dict_entry_t* _interpreter_find_word(interpreter_t* inter, const str_t n) {
  DO(i, inter->dict.l) if (str_eq(n, string_as_str(inter->dict.d[i].k))) { return &inter->dict.d[i]; }
  return NULL;
}

STATUS_T interpreter_token(interpreter_t* inter, token_t t) {
  switch (t.tok) {
    case TOK_EOF: STATUS_OK;
    case TOK_ERR: return status_errf("unexpected token: '%pS'", &t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = inter->stack->l;
      STATUS_OK;
    }
    case TOK_ARR_CLOSE: {
      assert(inter->arr_level);  // todo: report error
      size_t mark = inter->arr_marks[--inter->arr_level];
      assert(inter->stack->l >= mark);  // todo: report error
      return concatenate(inter->stack, inter->stack->l - mark);
    }
    case TOK_WORD: {
      dict_entry_t* e = _interpreter_find_word(inter, t.text);
      STATUS_CHECK(e, "unknown word '%pS'", &t.text);
      return interpreter_word(inter, e->v);
    }
    case TOK_I64: {
      stack_push(inter->stack, atom_t_i64(t.val.i));
      STATUS_OK;
    }
    case TOK_F64: {
      stack_push(inter->stack, atom_t_f64(t.val.d));
      STATUS_OK;
    }
    case TOK_STR: {
      size_t l = t.val.s.l;
      dim_t d = l;
      stack_push(inter->stack, array_new_t_c8(l, shape_1d(&d), t.val.s.p));
      STATUS_OK;
    }
    case TOK_QUOTE: {
      dict_entry_t* e = _interpreter_find_word(inter, t.val.s);
      STATUS_CHECK(e, "unknown word '%pS'", &t.text);
      stack_push(inter->stack, array_inc_ref(e->v));
      STATUS_OK;
    }
  }
  UNREACHABLE;
}

token_t _interpreter_next_token(interpreter_t* inter) { return next_token(&inter->line); }

STATUS_T interpreter_line(interpreter_t* inter, const char* s) {
  inter->line = s;

  for (;;) {
    token_t t = _interpreter_next_token(inter);
    if (t.tok == TOK_EOF) STATUS_OK;
    R_IF_ERR(interpreter_token(inter, t));
  }
  STATUS_OK;
}

// repl

STATUS_T repl() {
  size_t input_size = 0;
  own(char) input = NULL;

  own(interpreter_t) inter = interpreter_new();
  bool prompt = isatty(STDIN_FILENO);

  while (true) {
    if (prompt) {
      if (inter->arr_level) {
        printf("%ld> ", inter->arr_level);
      } else {
        stack_print(inter->stack);
        printf(" > ");
      }
    }

    if (getline(&input, &input_size, stdin) <= 0) STATUS_OK;
    status_t result = interpreter_line(inter, input);
    if (status_is_err(result)) {
      str_t msg = status_msg(result);
      fprintf(stderr, "ERROR: %pS\n", &msg);
      status_free(result);
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
      stack_clear(inter->stack);

      if (status_is_err(result)) {
        free(out);
        str_t msg = status_msg(result);
        out_size = asprintf(&out, "ERROR: %pS\n", &msg);
        status_free(result);
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

  STATUS_OK;
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