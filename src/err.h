#pragma once

#include <setjmp.h>

#include "str.h"

extern string_t global_err;
extern jmp_buf global_jmp_buf;

INLINE NORETURN PRINTF(1, 2) void __throw_errorf(const char* format, ...) {
  global_err = VA_ARGS_FWD(format, string_vnewf(format, args));
  longjmp(global_jmp_buf, 1);
}

#define ERROR(...) __throw_errorf(__VA_ARGS__)
#define CHECK(cond, ...) \
  if (!(cond)) ERROR(__VA_ARGS__)
