#pragma once

#include <stddef.h>
#include <string.h>
#include "common.h"

struct string_t;
struct str_t;

// unowned string
struct str_t {
  size_t      l;
  const char* p;

  inline str_t() : l(0), p(nullptr) {}
  inline str_t(nullptr_t) : l(0), p(nullptr) {}
  inline str_t(const char* b, const char* e) : l(e - b), p(b) {}
  inline str_t(const char* b, size_t l) : l(l), p(b) {}
  inline str_t(const char* s) : l(strlen(s)), p(s) {}

  inline size_t len() const { return l; }
  inline bool   empty() const { return !l; }

  inline string_t to_owned() const;

  inline bool operator==(const str_t& o) const { return l == o.l && !memcmp(p, o.p, l); }
};

INLINE void        str_fprint(const str_t s, FILE* f) { DO(i, s.len()) putc(*(s.p + i), f); }
INLINE void        str_print(const str_t s) { str_fprint(s, stdout); }
INLINE char        str_i(const str_t s, size_t i) { return *(s.p + i); }
INLINE const char* str_end(const str_t s) { return s.p + s.l; }
INLINE char*       str_toc(str_t s) {
  char* c = (char*)malloc(s.l + 1);
  memcpy(c, s.p, s.l);
  c[s.l] = 0;
  return c;
}
INLINE str_t str_memmem(const str_t h, const str_t n) {
  void* p = memmem(h.p, h.l, n.p, n.l);
  return p ? str_t((const char*)p, str_end(h)) : nullptr;
}
INLINE str_t str_memchr(const str_t h, char n) {
  const void* p = memchr(h.p, n, h.l);
  return p ? str_t((const char*)p, str_end(h)) : nullptr;
}
INLINE str_t str_skip(const str_t s, size_t b) {
  assert(b <= s.l);
  return str_t(s.p + b, s.l - b);
}
INLINE str_t str_slice(const str_t s, size_t b, size_t e) {
  assert(b <= s.l && e <= s.l);
  return str_t(s.p + b, s.p + e);
}
INLINE str_t str_first(const str_t s, size_t n) { return str_slice(s, 0, n); }
INLINE str_t str_last(const str_t s, size_t n) { return str_slice(s, s.l - n, s.l); }
INLINE bool  str_starts_with(const str_t s, const str_t p) { return s.l >= p.l && str_first(s, p.l) == p; }
INLINE bool  str_ends_with(const str_t s, const str_t p) { return s.l >= p.l && str_last(s, p.l) == p; }

// owned string
struct string_t {
 private:
  friend struct str_t;
  string_t(const string_t&) = delete;
  inline string_t(size_t l, const char* p) : l(l), p(new char[l]) { memcpy((void*)this->p, p, l); }

 public:
  inline string_t() : l(0), p(nullptr) {}
  inline string_t(string_t&& o) : l(o.l), p(o.p) {
    o.l = 0;
    o.p = nullptr;
  }
  explicit inline string_t(const char* s) : l(strlen(s)), p(new char[l]) { memcpy((void*)this->p, s, l); }
  inline ~string_t() { delete[] p; }

  inline string_t clone() const {
    char* p = new char[l];
    memcpy(p, this->p, l);
    return string_t(l, p);
  }
  inline operator str_t() const { return str_t(p, l); }

  inline string_t& operator=(string_t&& o) {
    delete[] p;
    l   = o.l;
    p   = o.p;
    o.l = 0;
    o.p = nullptr;
    return *this;
  }

  inline string_t& operator=(nullptr_t) {
    delete[] p;
    l = 0;
    p = nullptr;
    return *this;
  }

  size_t      l;
  const char* p;
};

static_assert(sizeof(string_t) == sizeof(str_t), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, l) == offsetof(str_t, l), "string_t & str_t binary layout should match");
static_assert(offsetof(string_t, p) == offsetof(str_t, p), "string_t & str_t binary layout should match");

INLINE void string_free(string_t s) { free((char*)s.p); }

inline string_t str_t::to_owned() const { return string_t(l, p); }