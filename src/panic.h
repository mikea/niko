#pragma once

#include <setjmp.h>
#include "common.h"
#include "str.h"

extern string_t __panic_message;
extern jmp_buf  global_jmp_buf;

NORETURN PRINTF(1, 2) void panicf(const char* format, ...);

#define CHECK(cond, ...) \
  if (!(cond)) panicf(__VA_ARGS__)

typedef void(unwind_t)(void* ctx, void* ptr);

typedef struct {
  unwind_t* h;
  void*     ctx;
  void*     p;
} unwind_entry_t;

GEN_VECTOR(unwind_stack, unwind_entry_t);

extern unwind_stack_t __unwind_stack;

INLINE void unwind_handler_push(unwind_t* h, void* ctx, void* p) {
  unwind_stack_push(&__unwind_stack, (unwind_entry_t){h, ctx, p});
}
INLINE void unwind_handler_pop(unwind_t* h, void* p) {
  UNUSED unwind_entry_t e = unwind_stack_pop(&__unwind_stack);
  assert(e.h == h);
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

#define PROTECT(t, x, h, ctx)        \
  ({                                 \
    t* _x = (x);                     \
    unwind_handler_push(h, ctx, _x); \
    _x;                              \
  })
