#include "farr.h"

void str_print(const str_t* s) {
  for (int i = 0; i < str_size(s); i++)
    putc(*(s->b + i), stdout);
}

int64_t str_parse_int64(const str_t* s) {
  int64_t result = 0;
  for (int i = 0; i < str_size(s); i++) {
    result *= 10;
    result += *(s->b + i) - '0';
  }
  return result;
}

// value type, passed as value
typedef struct {
  enum { INT64 } tag;
  union {
    int64_t i64;
  } data;
} val_t;

void val_print(val_t v) {
  switch (v.tag) {
    case INT64: {
      printf("%ld", v.data.i64);
      break;
    }
  }
}

typedef struct {
  val_t* bottom;
  size_t top;
  size_t size;
} stack_t;

void stack_push(stack_t* stack, val_t val) {
  if (stack->top == stack->size) {
    size_t new_size = (stack->size + 1) * 2;
    stack->bottom = reallocarray(stack->bottom, sizeof(val_t), new_size);
    stack->size = new_size;
  }
  stack->bottom[stack->top] = val;
  stack->top++;
}

void stack_print(stack_t* stack) {
  if (stack->top == 0)
    return;
  for (size_t i = 0; i < stack->top; i++) {
    if (i > 0)
      printf(" ");
    val_print(stack->bottom[i]);
  }
}

void interpret_i64(stack_t* stack, const str_t str) {
  int64_t i = str_parse_int64(&str);
  stack_push(stack, (val_t){INT64, .data.i64 = i});
}

void interpret(stack_t* stack, const char* s) {
  for (;;) {
    token_t t = next_token(&s);
    switch (t.tok) {
      case T_EOF:
        return;
      case T_ERROR: {
        printf("ERROR: unexpected token\n");
        break;
      }
      case T_INT64: {
        interpret_i64(stack, t.text);
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
    if (read < 0) {
      break;
    }
    interpret(&stack, input);
  }

  free(input);
  return 0;
}