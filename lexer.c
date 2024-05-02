#include "farr.h"

token_t next_token(const char** s) {
  const char* YYCURSOR = *s;

#define TOK(typ) \
  *s = YYCURSOR; \
  return (token_t) { .tok = (typ), .text = str_new(yytext, YYCURSOR) }

  for (;;) {
    const char* yytext = YYCURSOR;
    const char *YYMARKER;
    
    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCTYPE = char;

        integer = "-"?[1-9][0-9]*;
        float = ("-"?[0-9]+"."[0-9]*)|("-"?[0-9]+"."?[0-9]*[eE]"-"?[1-9][0-9]*);
        eof = [\x00];
        eow = [ \t\n\x00];
        spaces = [ \t\n]+;
        word = [^ ^\t^\n^\x00^1-9][^ \t\n\x00]*;

        eof { TOK(TOK_EOF); }
        spaces { continue; }
        integer / eow { TOK(TOK_I64); }
        float / eow { TOK(TOK_F64); }
        "[" / eow { TOK(TOK_ARR_OPEN); }
        "]" / eow { TOK(TOK_ARR_CLOSE); }
        word / eow { TOK(TOK_WORD); }
        * { TOK(TOK_ERR); }
    */
  }
}
