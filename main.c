#include "farr.h"

void str_print(const str_t* s) {
  for (int i = 0; i < str_size(s); i++) putc(*(s->b + i), stdout);
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

inline val_t val_int64(int64_t i) {
  return (val_t){.tag = INT64, .data.i64 = i};
}

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

val_t stack_pop(stack_t* stack) {
  assert(stack->top > 0);
  stack->top--;
  return stack->bottom[stack->top];
}

void stack_print(stack_t* stack) {
  if (stack->top == 0) return;
  for (size_t i = 0; i < stack->top; i++) {
    if (i > 0) printf(" ");
    val_print(stack->bottom[i]);
  }
}

val_t plus(val_t a, val_t b) {
  switch (a.tag) {
    case INT64: {
      switch (b.tag) {
        case INT64: {
          return val_int64(a.data.i64 + b.data.i64);
        }
      }
    }
  }
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
        stack_push(stack, val_int64(str_parse_int64(&t.text)));
        break;
      }
      case TOK_PLUS: {
        val_t b = stack_pop(stack);
        val_t a = stack_pop(stack);
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