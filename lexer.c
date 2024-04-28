#include "farr.h"

token_t next_token(const char** s) {
  const char* YYCURSOR = *s;

#define TOK(typ)                                    \
  *s = YYCURSOR;                                    \
  return (token_t) {                                \
    .tok = (typ), .text = str_new(yytext, YYCURSOR) \
  }

  for (;;) {
    const char* yytext = YYCURSOR;
    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCTYPE = char;

        number = [1-9][0-9]*;
        eof = [\x00\n];
        spaces = [ \t]+;

        eof { TOK(T_EOF); }
        number { TOK(T_INT64); }
        spaces { continue; }
        * { TOK(T_ERROR); }
    */
  }
#undef TOK
}
