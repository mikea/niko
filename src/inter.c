#include "inter.h"
#include "memory.h"
#include "prelude.h"
#include "words.h"

dict_t global_dict;

inter_t* inter_new() {
  inter_t* inter    = calloc(1, sizeof(inter_t));
  inter->stack      = stack_new();
  inter->comp_stack = stack_new();
  inter->out        = stdout;
  dict_reserve(&inter->dict, global_dict.s);
  DO(i, global_dict.s) {
    dict_entry_t* e = &global_dict.d[i];
    dict_push(&inter->dict, (dict_entry_t){copy(e->k), array_inc_ref(e->v), ENTRY_SYSTEM});
  }
  return inter;
}

void dict_free(dict_t* dict) {
  DO(i, dict->s) {
    string_free(dict->d[i].k);
    array_dec_ref(dict->d[i].v);
  }
  dict_shrink(dict);
}

void inter_free(inter_t* inter) {
  stack_free(inter->stack);
  stack_free(inter->comp_stack);
  dict_free(&inter->dict);
  free(inter);
}

void global_dict_add_new(dict_entry_t e) { dict_push(&global_dict, e); }

DESTRUCTOR void global_dict_free() { dict_free(&global_dict); }

array_t* concatenate(stack_t* stack, shape_t sh) {
  size_t len = shape_len(sh);
  if (!len) return array_alloc(T_I64, 0, sh);

  bool all_common            = true;
  borrow(array_t) top        = stack_peek(stack, 0);
  const shape_t common_shape = array_shape(top);
  const type_t  t            = top->t;
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
    a                      = array_alloc(t, shape_len(*new_shape), *new_shape);
    size_t stride          = type_sizeof(t, shape_len(common_shape));
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

ALWAYS_INLINE dict_entry_t* inter_lookup_entry(inter_t* inter, t_dict_entry e) {
  CHECK(e < inter->dict.s, "bad dict entry");
  return &inter->dict.d[e];
}

void inter_dict_entry(inter_t* inter, t_dict_entry e_idx) {
  stack_t*      stack = inter->stack;
  dict_entry_t* e     = inter_lookup_entry(inter, e_idx);
  if (e->f & ENTRY_CONST || e->f & ENTRY_VAR) {
    PUSH(e->v);
    return;
  }
  array_t* a = e->v;
  // DBG("%pS %pA", &inter->dict.d[e].k, a);
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
          borrow(array_t) x = stack_peek(stack, 0);
          f                 = (array_data_t_ffi(a))[x->t];
          CHECK(f, "%pT is not supported", &x->t);
          return f(inter, stack);
        }
        case 2: {
          borrow(array_t) y = stack_peek(stack, 0);
          borrow(array_t) x = stack_peek(stack, 1);
          f                 = ((t_ffi(*)[T_MAX])array_data(a))[x->t][y->t];
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
          case T_ARR:        NOT_IMPLEMENTED;
          case T_FFI:        NOT_IMPLEMENTED;
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

t_dict_entry inter_find_entry(inter_t* inter, const str_t n) {
  dict_t* dict = &inter->dict;
  DO(i, dict->s) {
    size_t j = dict->s - i - 1;
    if (str_eq(n, to_str(dict->d[j].k))) return j;
  }
  return dict->s;
}

str_t inter_next_word(inter_t* inter) {
  token_t next = _inter_next_token(inter);
  CHECK(next.tok == TOK_WORD, "word expected");
  return next.text;
}

void inter_token(inter_t* inter, token_t t) {
  stack_t* stack = inter->mode == MODE_COMPILE ? inter->comp_stack : inter->stack;

  switch (t.tok) {
    case TOK_EOF:      return;
    case TOK_ERR:      panicf("unexpected token: '%pS'", &t.text);
    case TOK_ARR_OPEN: {
      assert(inter->arr_level < sizeof(inter->arr_marks) / sizeof(inter->arr_marks[0]));
      inter->arr_marks[inter->arr_level++] = inter->stack->l;
      return;
    }
    case TOK_ARR_CLOSE: {
      assert(inter->arr_level);  // todo: report error
      size_t   mark  = inter->arr_marks[--inter->arr_level];
      stack_t* stack = inter->stack;
      assert(stack->l >= mark);  // todo: report error
      size_t n       = stack->l - mark;
      own(array_t) a = concatenate(stack, shape_1d(&n));
      PUSH(a);
      return;
    }
    case TOK_WORD: {
      if (str_eqc(t.text, ":")) {
        CHECK(inter->mode == MODE_INTERPRET, ": can be used only in interpret mode");
        str_t next = inter_next_word(inter);
        t_dict_entry prev  = inter_find_entry(inter, next);
        if (prev < inter->dict.s) {
          dict_entry_t* e = &inter->dict.d[prev];
          if (e->f & (ENTRY_CONST | ENTRY_SYSTEM)) panicf("`%pS` can't be redefined", &next);
        }
        inter->mode = MODE_COMPILE;
        assert(stack_is_empty(inter->comp_stack));
        inter->comp = copy(next);
        return;
      } else if (str_eqc(t.text, ";")) {
        CHECK(inter->mode == MODE_COMPILE, ": can be used only in compile mode");
        own(array_t) a = array_new_1d(T_ARR, inter->comp_stack->l, inter->comp_stack->data);

        t_dict_entry prev = inter_find_entry(inter, to_str(inter->comp));
        if (prev < inter->dict.s) {
          array_dec_ref(inter->dict.d[prev].v);
          inter->dict.d[prev].v = array_inc_ref(a);
        } else {
          dict_push(&inter->dict, (dict_entry_t){copy(inter->comp), array_inc_ref(a)});
        }

        inter->comp_stack->l = 0;
        inter->mode          = MODE_INTERPRET;
        string_free(inter->comp);
        return;
      } else if (str_eqc(t.text, "const")) {
        CHECK(inter->mode == MODE_INTERPRET, "const can be used only in interpret mode");
        str_t next = inter_next_word(inter);
        if (inter_find_entry(inter, next) < inter->dict.s) panicf("`%pS` can't be redefined", &next);
        POP(x);
        dict_push(&inter->dict, (dict_entry_t){copy(next), array_inc_ref(x), ENTRY_CONST});
        return;
      } else if (str_eqc(t.text, "var")) {
        CHECK(inter->mode == MODE_INTERPRET, "var can be used only in interpret mode");
        str_t next = inter_next_word(inter);
        if (inter_find_entry(inter, next) < inter->dict.s) panicf("`%pS` can't be redefined", &next);
        POP(x);
        dict_push(&inter->dict, (dict_entry_t){copy(next), array_inc_ref(x), ENTRY_VAR});
        return;
      } else {
        size_t e = inter_find_entry(inter, t.text);
        CHECK(e < inter->dict.s, "unknown word '%pS'", &t.text);
        switch (inter->mode) {
          case MODE_INTERPRET: return inter_dict_entry(inter, e);
          case MODE_COMPILE:   {
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
      size_t l       = t.val.s.l;
      dim_t  d       = l;
      own(array_t) a = array_new_t_c8(l, shape_1d(&d), t.val.s.p);
      PUSH(a);
      return;
    }
    case TOK_QUOTE: {
      size_t e = inter_find_entry(inter, t.val.s);
      CHECK(e < inter->dict.s, "unknown word '%pS'", &t.text);
      own(array_t) a = array_new_scalar_t_dict_entry(e);
      a->f |= FLAG_QUOTE;
      PUSH(a);
      return;
    }
  }
  UNREACHABLE;
}

inter_t* current;
inter_t* inter_current() { return current; }
void     inter_reset_current(bool*) { current = NULL; }

void inter_line(inter_t* inter, const char* s) {
  current = inter;
  UNUSED CLEANUP(inter_reset_current) bool b;

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
    *out_size  = asprintf(out, "ERROR: %pS\n", &e);
    inter->out = stdout;
    return;
  }

  PROTECTED(FILE, fout, open_memstream(out, out_size));
  inter->out = fout;
  inter_line(inter, line);
  inter->out = stdout;
}

void inter_load_prelude(inter_t* inter) {
  str_t prelude       = str_new_len((char*)__prelude_nk, __prelude_nk_len);
  own(char) c_prelude = str_toc(prelude);
  inter_line(inter, c_prelude);
}

void inter_reset(inter_t* inter) {
  stack_clear(inter->stack);
  stack_clear(inter->comp_stack);
  inter->arr_level = 0;
  inter->mode      = MODE_INTERPRET;
}

DEF_WORD("!", store) {
  POP(v);
  POP(x);
  dict_entry_t* e = inter_lookup_entry(inter, as_dict_entry(v));
  CHECK(e->f & ENTRY_VAR, "var expected");
  array_dec_ref(e->v);
  e->v = array_inc_ref(x);
}
