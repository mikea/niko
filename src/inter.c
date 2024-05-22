#include "inter.h"
#include "memory.h"
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

array_t* concatenate(stack_t* stack, shape_t sh) {
  size_t len = shape_len(sh);
  if (!len) return array_alloc(T_I64, 0, sh);

  bool all_common = true;
  borrow(array_t) top = stack_peek(stack, 0);
  const shape_t common_shape = array_shape(top);
  const type_t  t = top->t;
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
  return array_inc_ref(a);
}

// interpreter

token_t _inter_next_token(inter_t* inter) { return next_token(&inter->line); }

void inter_dict_entry(inter_t* inter, dict_entry_t* e) {
  stack_t* stack = inter->stack;
  array_t* a = e->v;
  switch (a->t) {
    case T_FFI: {
      t_ffi f;

      switch (a->r) {
        case 0: {
          f = *array_data_t_ffi(a);
          CHECK(f, "not implemented");
          return f(inter, stack);
        }
        case 1: {
          CHECK(stack_len(stack) >= 1, "stack underflow: 1 value expected");
          borrow(array_t) x = stack_peek(stack, 0);
          f = (array_data_t_ffi(a))[x->t];
          CHECK(f, "%pT is not supported", &x->t);
          return f(inter, stack);
        }
        case 2: {
          CHECK(stack_len(stack) >= 2, "stack underflow: 2 value expected");
          borrow(array_t) y = stack_peek(stack, 0);
          borrow(array_t) x = stack_peek(stack, 1);
          f = ((t_ffi(*)[T_MAX])array_data(a))[x->t][y->t];
          CHECK(f, "%pT %pT are not supported", &x->t, &y->t);
          return f(inter, stack);
        }
        default: panicf("unexpected ffi rank: %ld", a->r);
      }
    }
    case T_ARR: {
      DO_ARRAY(a, t_arr, i, p) {
        switch ((*p)->t) {
          case T_C8:
          case T_I64:
          case T_F64: {
            PUSH(*p);
            break;
          }
          case T_ARR: NOT_IMPLEMENTED;
          case T_FFI: NOT_IMPLEMENTED;
          case T_DICT_ENTRY: {
            if ((*p)->f & FLAG_QUOTE) PUSH(*p);
            else inter_dict_entry(inter, *array_data_t_dict_entry(*p));
            break;
          };
        }
      }
      return;
    }
    default: panicf("unexpected type: %d", a->t);
  }
}

dict_entry_t* _inter_find_word(inter_t* inter, const str_t n) {
  for (dict_entry_t* e = inter->dict; e; e = e->n)
    if (str_eq(n, string_as_str(e->k))) return e;
  for (dict_entry_t* e = global_dict; e; e = e->n)
    if (str_eq(n, string_as_str(e->k))) return e;
  return NULL;
}

void inter_token(inter_t* inter, token_t t) {
  stack_t* stack = inter->mode == MODE_COMPILE ? inter->comp_stack : inter->stack;

  switch (t.tok) {
    case TOK_EOF: return;
    case TOK_ERR: panicf("unexpected token: '%pS'", &t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = inter->stack->l;
      return;
    }
    case TOK_ARR_CLOSE: {
      assert(inter->arr_level);  // todo: report error
      size_t   mark = inter->arr_marks[--inter->arr_level];
      stack_t* stack = inter->stack;
      assert(stack->l >= mark);  // todo: report error
      size_t n = stack->l - mark;
      own(array_t) a = concatenate(stack, shape_1d(&n));
      PUSH(a);
      return;
    }
    case TOK_WORD: {
      if (str_eqc(t.text, ":")) {
        CHECK(inter->mode == MODE_INTERPRET, ": can be used only in interpret mode");
        inter->mode = MODE_COMPILE;
        assert(stack_is_empty(inter->comp_stack));
        token_t next = _inter_next_token(inter);
        CHECK(next.tok == TOK_WORD, "word expected");
        inter->comp = str_copy(next.text);
        return;
      } else if (str_eqc(t.text, ";")) {
        CHECK(inter->mode == MODE_COMPILE, ": can be used only in compile mode");
        own(array_t) a = array_new_1d(T_ARR, inter->comp_stack->l, inter->comp_stack->bottom);
        inter->comp_stack->l = 0;
        inter->dict = dict_entry_new(inter->dict, string_as_str(inter->comp), a);
        string_free(inter->comp);
        inter->mode = MODE_INTERPRET;
        return;
      } else {
        dict_entry_t* e = _inter_find_word(inter, t.text);
        CHECK(e, "unknown word '%pS'", &t.text);
        switch (inter->mode) {
          case MODE_INTERPRET: return inter_dict_entry(inter, e);
          case MODE_COMPILE: {
            own(array_t) a = array_new_scalar_t_dict_entry(e);
            PUSH(a);
            return;
          }
        }

        UNREACHABLE;
      }
    }
    case TOK_I64: {
      own(array_t) a = array_new_scalar_t_i64(t.val.i);
      PUSH(a);
      return;
    }
    case TOK_F64: {
      own(array_t) a = array_new_scalar_t_f64(t.val.d);
      PUSH(a);
      return;
    }
    case TOK_STR: {
      size_t l = t.val.s.l;
      dim_t  d = l;
      own(array_t) a = array_new_t_c8(l, shape_1d(&d), t.val.s.p);
      PUSH(a);
      return;
    }
    case TOK_QUOTE: {
      dict_entry_t* e = _inter_find_word(inter, t.val.s);
      CHECK(e, "unknown word '%pS'", &t.text);
      own(array_t) a = array_new_scalar_t_dict_entry(e);
      a->f |= FLAG_QUOTE;
      PUSH(a);
      return;
    }
  }
  UNREACHABLE;
}

void inter_line(inter_t* inter, const char* s) {
  inter->line = s;

  for (;;) {
    token_t t = _inter_next_token(inter);
    if (t.tok == TOK_EOF) return;
    inter_token(inter, t);
  }
}

void inter_line_capture_out(inter_t* inter, const char* line, char** out, size_t* out_size) {
  CATCH(e) {
    free(*out);
    *out_size = asprintf(out, "ERROR: %pS\n", &e);
    inter->out = stdout;
    return;
  }

  __attribute__((cleanup(FILE_cleanup_protected))) FILE* fout = ({
    FILE* _x = (open_memstream(out, out_size));
    unwind_handler_push(FILE_panic_handler, _x);
    _x;
  });
  inter->out = fout;
  inter_line(inter, line);
  inter->out = stdout;
}

void inter_load_prelude() {
  str_t prelude = str_new_len((char*)__prelude_nk, __prelude_nk_len);
  own(char) c_prelude = str_toc(prelude);
  own(inter_t) inter = inter_new();
  inter_line(inter, c_prelude);
  dict_entry_t* last = inter->dict;
  while (last->n != NULL) last = last->n;
  last->n = global_dict;
  global_dict = inter->dict;
  inter->dict = NULL;
}