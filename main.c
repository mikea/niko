#include "main.h"

struct str
{
    const char *b;
    const char *e;
};

inline size_t str_size(const struct str *s) { return s->e - s->b; }

void str_print(const struct str *s)
{
    for (int i = 0; i < str_size(s); i++)
        printf("%c", *(s->b + i));
}

bool lex(const char *s)
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

            number { str_print(&(struct str){yytext, YYCURSOR}); continue; }
            eof { printf("eof\n"); return true; }
            spaces { continue; }
            *      { printf("err\n"); return false; }
        */
    }
}

int main()
{
    char *input = NULL;
    size_t input_size = 0;

    while (true)
    {
        printf(" > ");
        ssize_t read = getline(&input, &input_size, stdin);
        if (read < 0)
        {
            break;
        }
        printf("%b\n", lex(input));
    }

    free(input);
    return 0;
}