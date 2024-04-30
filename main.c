#include "farr.h"

int64_t str_parse_i64(const str_t s) {
  int64_t result = 0;
  DO(i, str_len(s)) {
    result *= 10;
    result += *(s.b + i) - '0';
  }
  return result;
}

typedef struct {
  array_t** bottom;
  size_t top;
  size_t size;
} stack_t;

void stack_grow(stack_t* stack) {
  size_t new_size = (stack->size + 1) * 2;
  stack->bottom = reallocarray(stack->bottom, sizeof(array_t*), new_size);
  stack->size = new_size;
}

INLINE void stack_push(stack_t* stack, array_t* a) {
  if (stack->top == stack->size) stack_grow(stack);
  stack->bottom[stack->top++] = a;
}
INLINE stack_t* stack_assert_not_empty(stack_t* stack) {
  assert(stack->top > 0);
  return stack;
}
INLINE array_t* stack_pop(stack_t* stack) { return stack_assert_not_empty(stack)->bottom[--stack->top]; }
INLINE array_t* stack_peek(stack_t* stack) {
  return array_inc_ref(stack_assert_not_empty(stack)->bottom[stack->top - 1]);
}

void stack_print(stack_t* stack) {
  DO(i, stack->top) {
    if (i > 0) printf(" ");
    printf("%pA", stack->bottom[i]);
  }
}

void plus_i64_i64(size_t n, i64* restrict y, const i64* restrict a, const i64* restrict b) {
  DO(i, n) y[i] = a[i] + b[i];
}

WARN_UNUSED status_t plus(stack_t* stack) {
  array_t* b = stack_pop(stack);
  defer { array_dec_ref(b); }
  array_t* a = stack_pop(stack);
  defer { array_dec_ref(a); }

  assert(a->t == T_I64);
  assert(a->t == b->t);
  assert(a->r == b->r);
  assert(a->n == b->n);

  array_t* y = array_alloc(a->t, a->n, array_shape(a));
  plus_i64_i64(a->n, (i64*)array_mut_data(y), (const i64*)array_data(a), (const i64*)array_data(b));
  stack_push(stack, y);
  return status_ok();
}

WARN_UNUSED result_t shape(const array_t* x) {
  return result_ok(array_new(T_I64, x->r, (shape_t){1, &x->r}, array_shape(x).d));
}

WARN_UNUSED result_t len(const array_t* x) { return result_ok(atom_i64(x->n)); }

WARN_UNUSED result_t range(array_t* x) {
  assert(x->r == 0);      // todo
  assert(x->t == T_I64);  // todo
  i64 k = *(i64*)array_data(x);
  assert(k >= 0);
  size_t n = k;

  array_t* y = array_alloc(T_I64, n, (shape_t){1, &n});
  i64* ptr = (i64*)array_data(y);
  DO(i, n) ptr[i] = i;

  return result_ok(y);
}

WARN_UNUSED result_t zeros(array_t* x) {
  assert(x->r == 0);      // todo
  assert(x->t == T_I64);  // todo
  i64 k = *(i64*)array_data(x);
  assert(k >= 0);
  size_t n = k;

  array_t* y = array_alloc(T_I64, n, (shape_t){1, &n});
  i64* ptr = (i64*)array_data(y);
  DO(i, n) ptr[i] = 0;

  return result_ok(y);
}

WARN_UNUSED result_t ones(array_t* x) {
  assert(x->r == 0);      // todo
  assert(x->t == T_I64);  // todo
  i64 k = *(i64*)array_data(x);
  assert(k >= 0);
  size_t n = k;

  array_t* y = array_alloc(T_I64, n, (shape_t){1, &n});
  i64* ptr = (i64*)array_data(y);
  DO(i, n) ptr[i] = 1;

  return result_ok(y);
}

#define CALL1(fn)                         \
  {                                       \
    array_t* x = stack_pop(stack);        \
    defer { array_dec_ref(x); }           \
    result_t result = fn(x);              \
    if (result.ok) {                      \
      stack_push(stack, result.either.a); \
    } else {                              \
      return status_err(result.either.e); \
    }                                     \
  }

status_t interpret_word(stack_t* stack, const str_t w) {
  if (str_eqc(w, "+")) {
    return plus(stack);
  } else if (str_eqc(w, "shape")) {
    CALL1(shape);
  } else if (str_eqc(w, "len")) {
    CALL1(len);
  } else if (str_eqc(w, "range")) {
    CALL1(range);
  } else if (str_eqc(w, "zeros")) {
    CALL1(zeros);
  } else if (str_eqc(w, "ones")) {
    CALL1(ones);
  } else if (str_eqc(w, "dup")) {
    stack_push(stack, stack_peek(stack));
  } else if (str_eqc(w, "swap")) {
    array_t* a = stack_pop(stack);
    array_t* b = stack_pop(stack);
    stack_push(stack, a);
    stack_push(stack, b);
  } else if (str_eqc(w, "exit")) {
    printf("bye\n");
    exit(0);
  } else {
    fprintf(stderr, "unknown word: %pS\n", &w);
  }

  return status_ok();
}

#undef CALL1

void interpret(stack_t* stack, const char* s) {
  for (;;) {
    token_t t = next_token(&s);
    switch (t.tok) {
      case TOK_EOF: return;
      case TOK_WORD: {
        interpret_word(stack, t.text);
        break;
      }
      case TOK_INT64: {
        stack_push(stack, atom_i64(str_parse_i64(t.text)));
        break;
      }
      case TOK_ERR: {
        fprintf(stderr, "unexpected token: '%pS'\n", &t.text);
      }
    }
  }
}

int main() {
  register_printf_extensions();

  char* input = NULL;
  size_t input_size = 0;

  stack_t stack = {.bottom = NULL, .size = 0, .top = 0};

  while (true) {
    stack_print(&stack);
    printf(" > ");
    ssize_t read = getline(&input, &input_size, stdin);
    if (read < 0) { break; }
    interpret(&stack, input);
  }

  free(input);
  return 0;
}