#pragma once

#include "common.h"

// unowned string
typedef struct {
  size_t l;
  const char* p;
} str_t;

INLINE str_t str_new(const char* b, const char* e) { return (str_t){e - b, b}; }
INLINE str_t str_fromc(const char* c) { return (str_t){strlen(c), c}; }
INLINE size_t str_len(const str_t s) { return s.l; }
INLINE bool str_eq(const str_t s1, const str_t s2) { return s1.l == s2.l && !memcmp(s1.p, s2.p, s1.l); }
INLINE bool str_eqc(const str_t s, const char* c) { return str_eq(s, str_fromc(c)); }
INLINE void str_fprint(const str_t s, FILE* f) { DO(i, str_len(s)) putc(*(s.p + i), f); }
INLINE void str_print(const str_t s) { str_fprint(s, stdout); }
INLINE char str_i(const str_t s, size_t i) { return *(s.p + i); }
INLINE char* str_toc(str_t s) {
  char* c = malloc(s.l + 1);
  memcpy(c, s.p, s.l);
  c[s.l] = 0;
  return c;
}

// owned string
typedef struct {
  size_t l;
  const char* p;
} string_t;

static_assert(sizeof(string_t) == sizeof(str_t), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, l) == offsetof(str_t, l), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, p) == offsetof(str_t, p), "string_t & str_t binary layout should match");

INLINE void string_free(string_t s) { free((void*)s.p); }
INLINE string_t string_vnewf(const char* format, va_list args) {
  char* p;
  size_t l = vasprintf(&p, format, args);
  return (string_t){l, p};
}
INLINE PRINTF(1, 2) string_t string_newf(const char* format, ...) {
  return VA_ARGS_FWD(format, string_vnewf(format, args));
}
INLINE str_t string_as_str(string_t s) { return (str_t){s.l, s.p}; }
