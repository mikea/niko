#pragma once

#include <setjmp.h>
#include "common.h"
#include "str.h"

extern string_t __panic_message;
extern jmp_buf  global_jmp_buf;

NORETURN PRINTF(1, 2) void panicf(const char* format, ...);

#define CHECK(cond, ...) \
  if (!(cond)) panicf(__VA_ARGS__)

typedef void(unwind_t)(void* ptr);

typedef struct {
  unwind_t* c;
  void*     p;
} unwind_entry_t;

GEN_VECTOR(unwind_stack, unwind_entry_t);

extern unwind_stack_t __unwind_stack;

INLINE void unwind_handler_push(unwind_t* c, void* p) { unwind_stack_push(&__unwind_stack, (unwind_entry_t){c, p}); }
INLINE void unwind_handler_pop(unwind_t* c, void* p) {
  unwind_entry_t e = unwind_stack_pop(&__unwind_stack);
  assert(e.c == c);
  assert(e.p == p);
}

void        __unwind(size_t pos);
INLINE void __panic_message_cleanup(str_t*) { string_free(__panic_message); }

#define CATCH(msg)                                                            \
  for (size_t __before = 1, __pos = __unwind_stack.s; __before; __before = 0) \
    if (setjmp(global_jmp_buf))                                               \
      for (bool __after     = ({                                              \
             __unwind(__pos);                                             \
             true;                                                        \
           });                                                            \
           __after; __after = false)                                          \
        for (CLEANUP(__panic_message_cleanup) str_t msg = string_as_str(__panic_message); __after; __after = false)

#define PROTECT(t, x)                           \
  ({                                            \
    t* _x = (x);                                \
    unwind_handler_push(t##_panic_handler, _x); \
    _x;                                         \
  })
