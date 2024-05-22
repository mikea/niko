#include "panic.h"

#include "str.h"

string_t __panic_message;
jmp_buf global_jmp_buf;
unwind_stack_t __unwind_stack;

void __unwind(size_t pos) {
  assert(pos <= __unwind_stack.s);
  DO(i, __unwind_stack.s - pos) {
    unwind_entry_t* e = __unwind_stack.d + __unwind_stack.s - i - 1;
    e->c(e->p);
  }
  __unwind_stack.s = pos;
}

DESTRUCTOR void __panic_free() {
  assert(__unwind_stack.s == 0);
  unwind_stack_shrink(&__unwind_stack);
}

NORETURN void panic() { longjmp(global_jmp_buf, 1); }

NORETURN PRINTF(1, 2) void panicf(const char* format, ...) {
  __panic_message = VA_ARGS_FWD(format, string_vnewf(format, args));
  panic();
}