#include "farr.h"

#include <printf.h>

// %pA - print arrays
// %pS - print str/string

int p_modifier = -1;

size_t print_ptr(FILE* f, type_t t, const void* ptr) {
  switch (t) {
    case T_I64: return fprintf(f, "%ld", *(i64*)ptr);
    case T_F64: return fprintf(f, "%lf", *(f64*)ptr);
  }
  UNREACHABLE;
}

size_t print_array_impl(FILE* f, type_t t, shape_t s, const void* x) {
  if (s.r == 0) { return print_ptr(f, t, x); }
  size_t c = fprintf(f, "[ ");
  shape_t sub_shape = (shape_t){s.r - 1, s.d + 1};
  size_t stride = type_sizeof(t, shape_size(sub_shape));
  DO(i, *s.d) {
    c += print_array_impl(f, t, sub_shape, x + stride * i);
    c += fprintf(f, " ");
  }

  c += fprintf(f, "]");
  return c;
}

int printf_array(FILE* f, const struct printf_info* info, const void* const* args) {
  assert(info->user == p_modifier);  // p modifier expected
  const array_t* a = *((const array_t**)(args[0]));
  return print_array_impl(f, a->t, array_shape(a), array_data(a));
}

int printf_str(FILE* f, const struct printf_info* info, const void* const* args) {
  assert(info->user == p_modifier);  // p modifier expected
  const str_t* s = *((const str_t**)(args[0]));
  str_fprint(*s, f);
  return s->s;
}

int single_pointer_arginfo(const struct printf_info* __info, size_t n, int* argtypes, int* __size) {
  if (n > 0) argtypes[0] = PA_POINTER;
  return 1;
}

void register_printf_extensions() {
  register_printf_specifier('A', printf_array, single_pointer_arginfo);
  register_printf_specifier('S', printf_str, single_pointer_arginfo);
  p_modifier = register_printf_modifier(L"p");
}