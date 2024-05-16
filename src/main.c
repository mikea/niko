#include <getopt.h>
#include <unistd.h>

#include "niko.h"

dict_entry_t* global_dict = NULL;

DESTRUCTOR void global_dict_free() {
  dict_entry_free_chain(global_dict);
}

RESULT_T concatenate(stack_t* stack, size_t len) {
  if (!len) {
    dim_t d = len;
    RESULT_OK(array_move(array_alloc(T_I64, 0, shape_1d(&d))));
  }
  bool all_common = true;
  own(array_t) top = stack_peek(stack);
  const shape_t common_shape = array_shape(top);
  const type_t t = top->t;
  DO(i, len) {
    own(array_t) e = stack_i(stack, i);
    all_common &= e->t == t;
    all_common &= shape_eq(common_shape, array_shape(e));
  }

  own(array_t) a;
  if (!all_common) {
    dim_t d = len;
    a = array_alloc(T_ARR, len, shape_1d(&d));
    DO(i, len) { ((array_t**)array_mut_data(a))[i] = stack_i(stack, len - i - 1); }
  } else {
    own(shape_t) new_shape = shape_extend(common_shape, len);
    a = array_alloc(t, shape_len(*new_shape), *new_shape);
    size_t stride = type_sizeof(t, shape_len(common_shape));
    assert(array_data_sizeof(a) == stride * len);
    DO(i, len) {
      own(array_t) e = stack_i(stack, len - i - 1);
      assert(array_data_sizeof(e) == stride);
      memcpy(array_mut_data(a) + i * stride, array_data(e), stride);
    }
  }
  DO(i, len) stack_drop(stack);
  RESULT_OK(a);
}

// interpreter

interpreter_t* interpreter_new() {
  interpreter_t* inter = calloc(1, sizeof(interpreter_t));
  inter->stack = stack_new();
  inter->comp_stack = stack_new();
  inter->out = stdout;
  return inter;
}
void interpreter_free(interpreter_t* inter) {
  stack_free(inter->stack);
  stack_free(inter->comp_stack);
  dict_entry_free_chain(inter->dict);
  free(inter);
}
DEF_CLEANUP(interpreter_t, interpreter_free);

token_t _interpreter_next_token(interpreter_t* inter) { return next_token(&inter->line); }

STATUS_T interpreter_dict_entry(interpreter_t* inter, dict_entry_t* e) {
  array_t* a = e->v;
  switch (a->t) {
    case T_FFI: {
      t_ffi f;

      switch (a->r) {
        case 0: {
          f = *array_data_t_ffi(a);
          STATUS_CHECK(f, "not implemented");
          return f(inter, inter->stack);
        }
        case 1: {
          STATUS_CHECK(stack_len(inter->stack) >= 1, "stack underflow: 1 value expected");
          own(array_t) x = stack_peek(inter->stack);
          f = (array_data_t_ffi(a))[x->t];
          STATUS_CHECK(f, "%pT is not supported", &x->t);
          return f(inter, inter->stack);
        }
        case 2: {
          STATUS_CHECK(stack_len(inter->stack) >= 2, "stack underflow: 2 value expected");
          own(array_t) y = stack_i(inter->stack, 0);
          own(array_t) x = stack_i(inter->stack, 1);
          f = ((t_ffi(*)[T_MAX])array_data(a))[x->t][y->t];
          STATUS_CHECK(f, "%pT %pT are not supported", &x->t, &y->t);
          return f(inter, inter->stack);
        }
        default: return status_errf("unexpected ffi rank: %ld", a->r);
      }
    }
    case T_ARR: {
      DO_ARRAY(a, t_arr, i, p) {
        switch ((*p)->t) {
          case T_C8:
          case T_I64:
          case T_F64: {
            stack_push(inter->stack, *p);
            break;
          }
          case T_ARR: NOT_IMPLEMENTED;
          case T_FFI: NOT_IMPLEMENTED;
          case T_DICT_ENTRY: {
            STATUS_UNWRAP(interpreter_dict_entry(inter, *array_data_t_dict_entry(*p)));
            break;
          };
        }
      }
      STATUS_OK;
    }
    default: return status_errf("unexpected type: %d", a->t);
  }
}

dict_entry_t* _interpreter_find_word(interpreter_t* inter, const str_t n) {
  for (dict_entry_t* e = inter->dict; e; e = e->n) {
    if (str_eq(n, string_as_str(e->k))) return e;
  }
  for (dict_entry_t* e = global_dict; e; e = e->n) {
    if (str_eq(n, string_as_str(e->k))) return e;
  }
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
      own(array_t) a = RESULT_UNWRAP(concatenate(inter->stack, inter->stack->l - mark));
      stack_push(inter->stack, a);
      STATUS_OK;
    }
    case TOK_WORD: {
      if (str_eqc(t.text, ":")) {
        STATUS_CHECK(inter->mode == MODE_INTERPRET, ": can be used only in interpret mode");
        inter->mode = MODE_COMPILE;
        assert(stack_is_empty(inter->comp_stack));
        assert(inter->comp.v == NULL);
        token_t next = _interpreter_next_token(inter);
        STATUS_CHECK(next.tok == TOK_WORD, "word expected");
        inter->comp.k = str_copy(next.text);
        STATUS_OK;
      } else if (str_eqc(t.text, ";")) {
        STATUS_CHECK(inter->mode == MODE_COMPILE, ": can be used only in compile mode");
        inter->comp.v = array_move(array_new_1d(T_ARR, inter->comp_stack->l, inter->comp_stack->bottom));
        inter->comp_stack->l = 0;
        inter->dict = dict_entry_new(inter->dict, inter->comp.k, inter->comp.v);
        inter->comp.v = NULL;
        inter->mode = MODE_INTERPRET;
        STATUS_OK;
      } else {
        dict_entry_t* e = _interpreter_find_word(inter, t.text);
        STATUS_CHECK(e, "unknown word '%pS'", &t.text);
        switch (inter->mode) {
          case MODE_INTERPRET: return interpreter_dict_entry(inter, e);
          case MODE_COMPILE: {
            stack_push(inter->comp_stack, array_move(array_new_scalar_t_dict_entry(e)));
            STATUS_OK;
          }
        }

        UNREACHABLE;
      }
    }
    case TOK_I64: {
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, array_move(array_new_scalar_t_i64(t.val.i)));
          STATUS_OK;
        };
        case MODE_COMPILE: {
          stack_push(inter->comp_stack, array_move(array_new_scalar_t_i64(t.val.i)));
          STATUS_OK;
        }
      }
      UNREACHABLE;
    }
    case TOK_F64: {
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, array_move(array_new_scalar_t_f64(t.val.d)));
          STATUS_OK;
        };
        case MODE_COMPILE: {
          stack_push(inter->comp_stack, array_move(array_new_scalar_t_f64(t.val.d)));
          STATUS_OK;
        }
      }
      UNREACHABLE;
    }
    case TOK_STR: {
      size_t l = t.val.s.l;
      dim_t d = l;

      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, array_move(array_new_t_c8(l, shape_1d(&d), t.val.s.p)));
          STATUS_OK;
        };
        case MODE_COMPILE: {
          DBG("%pS", &t.text);
          NOT_IMPLEMENTED;
        }
      }

      UNREACHABLE;
    }
    case TOK_QUOTE: {
      dict_entry_t* e = _interpreter_find_word(inter, t.val.s);
      STATUS_CHECK(e, "unknown word '%pS'", &t.text);
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, array_move(array_new_scalar_t_dict_entry(e)));
          STATUS_OK;
        };
        case MODE_COMPILE: {
          NOT_IMPLEMENTED;
        }
      }
    }
  }
  UNREACHABLE;
}

STATUS_T interpreter_line(interpreter_t* inter, const char* s) {
  inter->line = s;

  for (;;) {
    token_t t = _interpreter_next_token(inter);
    if (t.tok == TOK_EOF) STATUS_OK;
    STATUS_UNWRAP(interpreter_token(inter, t));
  }
  STATUS_OK;
}

void interpreter_line_capture_out(interpreter_t* inter, const char* line, char** out, size_t* out_size) {
  FILE* fout = open_memstream(out, out_size);
  inter->out = fout;
  status_t result = interpreter_line(inter, line);
  inter->out = stdout;
  fclose(fout);

  if (status_is_err(result)) {
    free(*out);
    str_t msg = status_msg(result);
    *out_size = asprintf(out, "ERROR: %pS\n", &msg);
    status_free(result);
  }
}

// repl

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif
#ifndef COMPILE_TIME
#define COMPILE_TIME "unknown"
#endif

INLINE void stack_print_repl(stack_t* stack) {
  DO(i, stack->l) {
    if (i > 0) printf(" ");
    printf("%25pA", stack->bottom[i]);
  }
}

STATUS_T repl() {
  size_t input_size = 0;
  own(char) input = NULL;

  own(interpreter_t) inter = interpreter_new();
  bool prompt = isatty(STDIN_FILENO);

  if (prompt) printf("niko %s (%s)\n", GIT_DESCRIBE, COMPILE_TIME);

  while (true) {
    if (prompt) {
      if (inter->arr_level) {
        printf("%ld> ", inter->arr_level);
      } else {
        stack_print_repl(inter->stack);
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
  STATUS_CHECK(file, "test: can't open file: %s", fname);

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
      interpreter_line_capture_out(inter, line + 1, &out, &out_size);
      stack_clear(inter->stack);
      rest_out = out;
      continue;
    }

    if (memcmp(line, rest_out, read)) {
      fprintf(stderr, "ERROR %s:%ld : mismatched output, expected: '%s' actual: '%s' \n", fname, in_line_no, line,
              rest_out);
      rest_out = NULL;
    } else {
      rest_out += read;
    }
  }

  if (rest_out && *rest_out) fprintf(stderr, "ERROR %s:%ld : umatched output: '%s'\n", fname, in_line_no, rest_out);

  STATUS_OK;
}

STATUS_T eval(const char* stmt) {
  own(interpreter_t) inter = interpreter_new();
  return interpreter_line(inter, stmt);
}

STATUS_T markdown(const char* fname) {
  own(FILE) file = fopen(fname, "r");
  STATUS_CHECK(file, "can't open file: %s", fname);

  char tmp_template[] = "/tmp/niko-XXXXXX";
  int temp_fd = mkstemp(tmp_template);
  STATUS_CHECK(temp_fd, "can't create temp file");
  own(FILE) temp = fdopen(temp_fd, "w");

  const str_t code_start = str_from_c("```nk\n");
  const str_t code_end = str_from_c("```");

  own(interpreter_t) inter = interpreter_new();

  size_t len = 0;
  own(char) line = NULL;

  size_t read;
  size_t line_no = 0;

  own(char) out = NULL;
  size_t out_size;

  bool in_code = false;

  while ((read = getline(&line, &len, file)) != -1) {
    line_no++;
    str_t l = str_from_c(line);

    if (in_code) {
      if (str_starts_with(l, code_end)) {
        fprintf(temp, "%pS", &l);
        in_code = false;
      } else if (*l.p == '>') {
        fprintf(temp, "%pS", &l);
        interpreter_line_capture_out(inter, l.p + 1, &out, &out_size);
        fprintf(temp, "%s", out);
      } else {
      };
    } else {
      fprintf(temp, "%pS", &l);
      if (str_ends_with(l, code_start)) in_code = true;
    }
  }

  FILE_cleanup(&file);
  FILE_cleanup(&temp);
  STATUS_CHECK(!rename(tmp_template, fname), "rename error");
  STATUS_OK;
}

// main

int main(int argc, char* argv[]) {
  char *t = NULL, *e = NULL, *m = NULL;
  bool v = false;

  int opt;
  while ((opt = getopt(argc, argv, "vht:e:m:")) != -1) {
    switch (opt) {
      case 't': t = optarg; break;
      case 'v': v = true; break;
      case 'e': e = optarg; break;
      case 'm': m = optarg; break;
      case 'h':
      default:
        fprintf(stderr, "Usage: %s | %s -t test_file [-v] | %s -e stmt | %s -h \n", argv[0], argv[0], argv[0], argv[0]);
        exit(1);
    }
  }

  status_t s;
  if (t) s = test(t, v);
  else if (m) s = markdown(m);
  else if (e) s = eval(e);
  else s = repl();
  if (status_is_ok(s)) return 0;
  status_print_error(s);
  return 1;
}