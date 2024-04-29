#include "farr.h"

void str_print(const str_t* s) {
  for (int i = 0; i < str_size(s); i++) putc(*(s->b + i), stdout);
}

int64_t str_parse_i64(const str_t* s) {
  int64_t result = 0;
  for (int i = 0; i < str_size(s); i++) {
    result *= 10;
    result += *(s->b + i) - '0';
  }
  return result;
}

void array_print(array_t* arr) {
  assert(arr->d == 0);  // todo
  assert(arr->n == 1);  // todo

  switch (arr->t) {
    case T_I64: printf("%ld", *array_data_i64(arr)); break;
    case T_F64: printf("%lf", *array_data_f64(arr)); break;
  }
}

typedef struct {
  array_t** bottom;
  size_t top;
  size_t size;
} stack_t;

void stack_push(stack_t* stack, array_t* a) {
  if (stack->top == stack->size) {
    size_t new_size = (stack->size + 1) * 2;
    stack->bottom = reallocarray(stack->bottom, sizeof(array_t*), new_size);
    stack->size = new_size;
  }
  stack->bottom[stack->top] = a;
  stack->top++;
}

array_t* stack_pop(stack_t* stack) {
  assert(stack->top > 0);
  stack->top--;
  return stack->bottom[stack->top];
}

void stack_print(stack_t* stack) {
  if (stack->top == 0) return;
  for (size_t i = 0; i < stack->top; i++) {
    if (i > 0) printf(" ");
    array_print(stack->bottom[i]);
  }
}

array_t* plus(array_t* a, array_t* b) {
  assert(false);  // todo
}

void interpret(stack_t* stack, const char* s) {
  for (;;) {
    token_t t = next_token(&s);
    switch (t.tok) {
      case TOK_EOF: return;
      case TOK_ERROR: {
        printf("ERROR: unexpected token: ");
        str_print(&t.text);
        printf("\n");
        break;
      }
      case TOK_INT64: {
        stack_push(stack, atom_i64(str_parse_i64(&t.text)));
        break;
      }
      case TOK_PLUS: {
        array_t* b = stack_pop(stack);
        array_t* a = stack_pop(stack);
        stack_push(stack, plus(a, b));
        break;
      }
    }
  }
}

int main() {
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