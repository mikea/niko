#pragma once

#include "str.h"

// status
typedef struct {
  bool ok;
  union {
    string_t s;
  } u;
} status_t;
#define STATUS_T WARN_UNUSED status_t
INLINE void status_free(status_t s) {
  if (!s.ok) string_free(s.u.s);
}
INLINE status_t status_ok() { return (status_t){true}; }
INLINE status_t status_err(string_t msg) { return (status_t){.ok = false, .u.s = msg}; }
INLINE PRINTF(1, 2) status_t status_errf(const char* format, ...) {
  return status_err(VA_ARGS_FWD(format, string_vnewf(format, args)));
}
INLINE bool status_is_ok(status_t s) { return s.ok; }
INLINE bool status_is_err(status_t s) { return !s.ok; }
INLINE status_t status_assert_err(status_t s) {
  assert(!s.ok);
  return s;
}
INLINE str_t status_msg(status_t s) { return string_as_str(status_assert_err(s).u.s); }
INLINE void status_print_error(status_t s) {
  str_t msg = status_msg(s);
  fprintf(stderr, "ERROR: %pS\n", &msg);
}

#define R_OK return status_ok()
#define R_ERRF(...) return status_errf(__VA_ARGS__)

#define R_IF_ERR(expr)                                \
  do {                                                \
    status_t __status__ = (expr);                     \
    if (status_is_err(__status__)) return __status__; \
  } while (0)

#define R_CHECK(cond, ...) if (!(cond)) R_ERRF(__VA_ARGS__)