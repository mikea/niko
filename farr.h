#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// unowned string
typedef struct {
  const char* b;
  size_t s;
} str_t;

inline str_t str_new(const char* b, const char* e) { return (str_t){b, e - b}; }
inline size_t str_size(const str_t* s) { return s->s; }

// token
typedef struct {
  enum { TOK_ERROR, TOK_EOF, TOK_INT64, TOK_PLUS } tok;
  str_t text;
} token_t;

token_t next_token(const char** s);