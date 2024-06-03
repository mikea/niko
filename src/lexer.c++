#include "inter.h"

int64_t str_parse_i64(const str_t s) {
  own(char) c = str_toc(s);
  return strtol(c, NULL, 10);
}
double str_parse_f64(const str_t s) {
  own(char) c = str_toc(s);
  return strtod(c, NULL);
}

token_t next_token(const char** s) {
  const char* YYCURSOR = *s;

#define TOK(typ) \
  *s = YYCURSOR; \
  return { .tok = (typ), .text = str_new(yytext, YYCURSOR) }

#define TOK_VAL(typ, val) \
  *s = YYCURSOR;          \
  return { .tok = (typ), .text = str_new(yytext, YYCURSOR), val }

  for (;;) {
    const char* yytext = YYCURSOR;
    const char* YYMARKER;

    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCTYPE = char;

        integer = "-"?[0-9][0-9]*;
        float = ("-"?[0-9]+"."[0-9]*)|("-"?[0-9]+"."?[0-9]*[eE]"-"?[1-9][0-9]*);
        str = "\""[^"]*"\"";
        eof = [\x00];
        eow = [ \t\n\x00];
        spaces = [ \t\n]+;
        word = [^ ^\t^\n^\x00^\(][^ \t\n\x00]*;
        comment = "("[^\)^\x00]*")";

        eof { TOK(TOK_EOF); }
        comment { continue; }
        spaces { continue; }
        integer / eow { TOK_VAL(TOK_I64, .val = {.i = str_parse_i64(str_new(yytext, YYCURSOR))}); }
        float / eow { TOK_VAL(TOK_F64, .val = {.d = str_parse_f64(str_new(yytext, YYCURSOR))}); }
        "[" / eow { TOK(TOK_ARR_OPEN); }
        "]" / eow { TOK(TOK_ARR_CLOSE); }
        str / eow { TOK_VAL(TOK_STR, .val{.s = str_new(yytext+1, YYCURSOR-1)}); }
        word"'" / eow { TOK_VAL(TOK_QUOTE, .val{.s = str_new(yytext, YYCURSOR-1)}); }
        word / eow { TOK(TOK_WORD); }
        * { TOK(TOK_ERR); }
    */
  }
}
