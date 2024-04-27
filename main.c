#include "main.h"

typedef struct
{
    const char *b;
    size_t s;
} Str;

inline size_t str_size(const Str *s) { return s->s; }

void str_print(const Str *s)
{
    for (int i = 0; i < str_size(s); i++)
        putc(*(s->b + i), stdout);
}

int64_t str_parse_int64(const Str *s) {
    int64_t result = 0;
    for (int i = 0; i < str_size(s); i++) {
        result *= 10;
        result += *(s->b + i) - '0';
    }
    return result;
}

Str str_new(const char*b, const char*e) { return (Str){ b, e-b};}

typedef struct {
    enum { INT64 } tag;
    union {
        int64_t i64;
    } data;
} Val;

typedef struct {
    Val* bottom;
    size_t top;
    size_t size;
} Stack;

void stack_push(Stack* stack, Val val) {
    assert(false);
}

void interpret_i64(Stack* stack, const Str str) {
    int64_t i = str_parse_int64(&str);
    stack_push(stack, (Val){INT64, .data.i64 = i});
} 

bool interpret(Stack* stack, const char *s)
{
    const char *YYCURSOR = s;

    for (;;)
    {
        const char *yytext = YYCURSOR;
        /*!re2c
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = char;

            number = [1-9][0-9]*;
            eof = [\x00\n];
            spaces = [ \t]+;

            number { interpret_i64(stack, str_new(yytext, YYCURSOR)); continue; }
            eof { return true; }
            spaces { continue; }
            * { printf("err\n"); return false; }
        */
    }
}

int main()
{
    char *input = NULL;
    size_t input_size = 0;

    Stack stack = {.bottom=NULL, .size = 0, .top = 0};

    while (true)
    {
        printf(" > ");
        ssize_t read = getline(&input, &input_size, stdin);
        if (read < 0)
        {
            break;
        }
        if (interpret(&stack, input))
        {
            printf("OK\n");
        } else {
            printf("ERROR\n");
        }
    }

    free(input);
    return 0;
}