#include "inter.h"
#include "prelude.h"

inter_t* inter_new() {
  inter_t* inter = calloc(1, sizeof(inter_t));
  inter->stack = stack_new();
  inter->comp_stack = stack_new();
  inter->out = stdout;
  return inter;
}

void inter_free(inter_t* inter) {
  stack_free(inter->stack);
  stack_free(inter->comp_stack);
  dict_entry_free_chain(inter->dict);
  free(inter);
}

dict_entry_t* global_dict = NULL;

DESTRUCTOR void global_dict_free() { dict_entry_free_chain(global_dict); }

RESULT_T concatenate(stack_t* stack, shape_t sh) {
  size_t len = shape_len(sh);
  if (!len) RESULT_OK(array_move(array_alloc(T_I64, 0, sh)));

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
    a = array_alloc(T_ARR, len, sh);
    DO(i, len) { ((array_t**)array_mut_data(a))[i] = stack_i(stack, len - i - 1); }
  } else {
    own(shape_t) new_shape = shape_extend(sh, common_shape);
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

token_t _inter_next_token(inter_t* inter) { return next_token(&inter->line); }

STATUS_T inter_dict_entry(inter_t* inter, dict_entry_t* e) {
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
            if ((*p)->f & FLAG_QUOTE) stack_push(inter->stack, *p);
            else STATUS_UNWRAP(inter_dict_entry(inter, *array_data_t_dict_entry(*p)));
            break;
          };
        }
      }
      STATUS_OK;
    }
    default: return status_errf("unexpected type: %d", a->t);
  }
}

dict_entry_t* _inter_find_word(inter_t* inter, const str_t n) {
  for (dict_entry_t* e = inter->dict; e; e = e->n)
    if (str_eq(n, string_as_str(e->k))) return e;
  for (dict_entry_t* e = global_dict; e; e = e->n)
    if (str_eq(n, string_as_str(e->k))) return e;
  return NULL;
}

STATUS_T inter_token(inter_t* inter, token_t t) {
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
      size_t n = inter->stack->l - mark;
      own(array_t) a = RESULT_UNWRAP(concatenate(inter->stack, shape_1d(&n)));
      stack_push(inter->stack, a);
      STATUS_OK;
    }
    case TOK_WORD: {
      if (str_eqc(t.text, ":")) {
        STATUS_CHECK(inter->mode == MODE_INTERPRET, ": can be used only in interpret mode");
        inter->mode = MODE_COMPILE;
        assert(stack_is_empty(inter->comp_stack));
        token_t next = _inter_next_token(inter);
        STATUS_CHECK(next.tok == TOK_WORD, "word expected");
        inter->comp = str_copy(next.text);
        STATUS_OK;
      } else if (str_eqc(t.text, ";")) {
        STATUS_CHECK(inter->mode == MODE_COMPILE, ": can be used only in compile mode");
        own(array_t) a = array_new_1d(T_ARR, inter->comp_stack->l, inter->comp_stack->bottom);
        inter->comp_stack->l = 0;
        inter->dict = dict_entry_new(inter->dict, string_as_str(inter->comp), a);
        string_free(inter->comp);
        inter->mode = MODE_INTERPRET;
        STATUS_OK;
      } else {
        dict_entry_t* e = _inter_find_word(inter, t.text);
        STATUS_CHECK(e, "unknown word '%pS'", &t.text);
        switch (inter->mode) {
          case MODE_INTERPRET: return inter_dict_entry(inter, e);
          case MODE_COMPILE: {
            own(array_t) a = array_new_scalar_t_dict_entry(e);
            stack_push(inter->comp_stack, a);
            STATUS_OK;
          }
        }

        UNREACHABLE;
      }
    }
    case TOK_I64: {
      own(array_t) a = array_new_scalar_t_i64(t.val.i);
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, a);
          STATUS_OK;
        };
        case MODE_COMPILE: {
          stack_push(inter->comp_stack, a);
          STATUS_OK;
        }
      }
      UNREACHABLE;
    }
    case TOK_F64: {
      own(array_t) a = array_new_scalar_t_f64(t.val.d);
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, a);
          STATUS_OK;
        };
        case MODE_COMPILE: {
          stack_push(inter->comp_stack, a);
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
      dict_entry_t* e = _inter_find_word(inter, t.val.s);
      STATUS_CHECK(e, "unknown word '%pS'", &t.text);
      own(array_t) a = array_new_scalar_t_dict_entry(e);
      a->f |= FLAG_QUOTE;
      switch (inter->mode) {
        case MODE_INTERPRET: {
          stack_push(inter->stack, a);
          STATUS_OK;
        };
        case MODE_COMPILE: {
          stack_push(inter->comp_stack, a);
          STATUS_OK;
        }
      }
      UNREACHABLE;
    }
  }
  UNREACHABLE;
}

STATUS_T inter_line(inter_t* inter, const char* s) {
  inter->line = s;

  for (;;) {
    token_t t = _inter_next_token(inter);
    if (t.tok == TOK_EOF) STATUS_OK;
    STATUS_UNWRAP(inter_token(inter, t));
  }
  STATUS_OK;
}

void inter_line_capture_out(inter_t* inter, const char* line, char** out, size_t* out_size) {
  FILE* fout = open_memstream(out, out_size);
  inter->out = fout;
  status_t result = inter_line(inter, line);
  inter->out = stdout;
  fclose(fout);

  if (status_is_err(result)) {
    free(*out);
    str_t msg = status_msg(result);
    *out_size = asprintf(out, "ERROR: %pS\n", &msg);
    status_free(result);
  }
}

void inter_load_prelude() {
  str_t prelude = str_new_len((char*)__prelude_nk, __prelude_nk_len);
  own(char) c_prelude = str_toc(prelude);
  own(inter_t) inter = inter_new();
  STATUS_EXPECT(inter_line(inter, c_prelude));
  dict_entry_t* last = inter->dict;
  while (last->n != NULL) last = last->n;
  last->n = global_dict;
  global_dict = inter->dict;
  inter->dict = NULL;
}