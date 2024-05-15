#pragma once

#include "common.h"
#include <stddef.h>
#include <string.h>

// unowned string
typedef struct {
  size_t l;
  const char* p;
} str_t;

INLINE str_t str_empty() { return (str_t){0, NULL}; }
INLINE str_t str_new(const char* b, const char* e) { return (str_t){e - b, b}; }
INLINE str_t str_from_c(const char* c) { return (str_t){strlen(c), c}; }
INLINE size_t str_len(const str_t s) { return s.l; }
INLINE bool str_is_empty(const str_t s) { return !s.l; }
INLINE bool str_eq(const str_t s1, const str_t s2) { return s1.l == s2.l && !memcmp(s1.p, s2.p, s1.l); }
INLINE bool str_eqc(const str_t s, const char* c) { return str_eq(s, str_from_c(c)); }
INLINE void str_fprint(const str_t s, FILE* f) { DO(i, str_len(s)) putc(*(s.p + i), f); }
INLINE void str_print(const str_t s) { str_fprint(s, stdout); }
INLINE char str_i(const str_t s, size_t i) { return *(s.p + i); }
INLINE const char* str_end(const str_t s) { return s.p + s.l; }
INLINE char* str_toc(str_t s) {
  char* c = malloc(s.l + 1);
  memcpy(c, s.p, s.l);
  c[s.l] = 0;
  return c;
}
INLINE str_t str_memmem(const str_t h, const str_t n) {
  void* p = memmem(h.p, h.l, n.p, n.l);
  return p ? str_new(p, str_end(h)) : str_empty();
}
INLINE str_t str_memchr(const str_t h, char n) {
  void* p = memchr(h.p, n, h.l);
  return p ? str_new(p, str_end(h)) : str_empty();
}
INLINE str_t str_skip(const str_t s, size_t b) {
  assert(b <= s.l);
  return (str_t){.l = s.l - b, .p = s.p + b};
}
INLINE str_t str_slice(const str_t s, size_t b, size_t e) {
  assert(b <= s.l && e <= s.l);
  return str_new(s.p + b, s.p + e);
}
INLINE str_t str_first(const str_t s, size_t n) { return str_slice(s, 0, n); }
INLINE str_t str_last(const str_t s, size_t n) { return str_slice(s, s.l - n, s.l); }
INLINE bool str_starts_with(const str_t s, const str_t p) { return s.l >= p.l && str_eq(str_first(s, p.l), p); }
INLINE bool str_ends_with(const str_t s, const str_t p) { return s.l >= p.l && str_eq(str_last(s, p.l), p); }

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
INLINE string_t str_copy(const str_t s) {
  char* p = malloc(s.l);
  memcpy(p, s.p, s.l);
  return (string_t){.l = s.l, .p = p};
}
INLINE string_t string_from_c(const char* s) { return str_copy(str_from_c(s)); }