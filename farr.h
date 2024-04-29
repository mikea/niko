#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int64_t i64;
typedef double f64;
static_assert(sizeof(f64) == sizeof(i64));

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

// type
typedef enum { T_I64, T_F64 } type_t;

inline size_t type_sizeof(type_t t) {
  switch (t) {
    case T_I64: return sizeof(i64);
    case T_F64: return sizeof(f64);
  }
}

// arrays: header + rank + data
typedef struct {
  type_t t;   // type
  size_t rc;  // ref count
  size_t n;   // number of elements
  size_t d;   // dimensions
} array_t;

inline void* array_data(array_t* arr) { return (void*)(arr + 1) + sizeof(size_t) * arr->d; }
inline i64* array_data_i64(array_t* arr) { return (i64*)array_data(arr); }
inline f64* array_data_f64(array_t* arr) { return (f64*)array_data(arr); }

inline array_t* array_alloc(type_t t, size_t n, size_t d, size_t* r) {
  array_t* a = malloc(sizeof(array_t) + d * sizeof(size_t) + n * type_sizeof(t));
  a->t = t;
  a->rc = 1;
  a->n = n;
  a->d = d;
  if (d > 0) memcpy(a + sizeof(array_t), r, d * sizeof(size_t));
  return a;
}

inline array_t* atom_i64(i64 i) {
  array_t* a = array_alloc(T_I64, 1, 0, NULL);
  *array_data_i64(a) = i;
  return a;
}