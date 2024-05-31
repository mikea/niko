#include <printf.h>

#include "array.h"
#include "inter.h"

// %pp - void*
// %pA - array_t*
// %pS - str_t*/string_t*
// %pH - shape_t*
// %pT - type_t*

int p_modifier = -1;

size_t print_ptr(FILE* f, type_t t, flags_t fl, const void* ptr) {
  switch (t) {
    case T_I64: return fprintf(f, "%ld", *(i64*)ptr);
    case T_F64: {
      own(char) s = NULL;
      (void)!asprintf(&s, "%.15g", *(f64*)ptr);
      if (strchr(s, '.') || strchr(s, 'e')) return fprintf(f, "%s", s);
      else return fprintf(f, "%s.", s);
    }
    case T_C8:         UNREACHABLE;
    case T_ARR:        return fprintf(f, "%pA", *(array_t**)ptr);
    case T_FFI:        return fprintf(f, "<native_function>");
    case T_DICT_ENTRY: {
      size_t        idx = *(t_dict_entry*)ptr;
      dict_entry_t* e   = &inter_current()->dict.d[idx];
      if (fl & FLAG_QUOTE) return fprintf(f, "%pS'", &(e->k));
      else return fprintf(f, "%pS", &(e->k));
    }
  }
  UNREACHABLE;
}

size_t print_array_impl(FILE* f, type_t t, flags_t fl, size_t n, const void* x, size_t w) {
  if (fl & FLAG_ATOM) return print_ptr(f, t, fl, x);
  size_t c = 0;
  if (t == T_C8) {
    c += fprintf(f, "\"");
    DO(i, n) putc(((const char*)x)[i], f);
    c += n;
    c += fprintf(f, "\"");
    return c;
  }
  c             += fprintf(f, "[ ");
  size_t stride  = type_sizeof(t, 1);
  DO(i, n) {
    if (w < c + 4) {
      c += fprintf(f, "... ");
      break;
    }
    c += print_ptr(f, t, fl, x + stride * i);
    c += fprintf(f, " ");
  }
  c += fprintf(f, "]");
  return c;
}

int printf_array(FILE* f, const struct printf_info* info, const void* const* args) {
  assert(info->user == p_modifier);  // p modifier expected
  const array_t* a = *((const array_t**)(args[0]));
  return print_array_impl(f, a->t, a->f, a->n, array_data(a), info->width ? info->width : SIZE_MAX);
}

int printf_str(FILE* f, const struct printf_info* info, const void* const* args) {
  assert(info->user == p_modifier);  // p modifier expected
  const str_t* s = *((const str_t**)(args[0]));
  str_fprint(*s, f);
  return s->l;
}

int printf_type(FILE* f, const struct printf_info* info, const void* const* args) {
  assert(info->user == p_modifier);  // p modifier expected
  const type_t* t = *((const type_t**)(args[0]));
  return fprintf(f, "%s", type_name(*t));
}

int single_pointer_arginfo(const struct printf_info* __info, size_t n, int* argtypes, int* __size) {
  if (n) argtypes[0] = PA_POINTER;
  return 1;
}

ATTR(constructor(9999)) void register_printf_extensions() {
  register_printf_specifier('A', printf_array, single_pointer_arginfo);
  register_printf_specifier('S', printf_str, single_pointer_arginfo);
  register_printf_specifier('T', printf_type, single_pointer_arginfo);
  p_modifier = register_printf_modifier(L"p");
}