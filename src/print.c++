#include <sstream>

#include "print.h"
#include "inter.h"

using std::format_to;

ttT T format_atom(type_t t, flags_t f, const void* ptr, T out) {
  switch (t) {
    case T_I64: return format_to(out, "{}", *(i64*)ptr);
    case T_F64: {
      char* s = NULL;
      (void)!asprintf(&s, "%.15g", *(f64*)ptr);
      defer { free(s); };
      if (strchr(s, '.') || strchr(s, 'e')) return format_to(out, "{}", s);
      else return format_to(out, "{}.", s);
    }
    case T_C8:         NOT_IMPLEMENTED; 
    case T_ARR:        return format_to(out, "{}", *(array_t**)ptr);
    case T_FFI:        NOT_IMPLEMENTED;  // return fprintf(f, "<native_function>");
    case T_DICT_ENTRY: {
        size_t        idx = *(t_dict_entry*)ptr;
        dict_entry_t* e   = &inter_current()->dict.d[idx];
        if (f & FLAG_QUOTE) return format_to(out, "{}'", e->k);
        else return format_to(out, "{}", e->k);
    }
  }
  UNREACHABLE;
}

std::format_context::iterator std::formatter<array_t*>::format(const array_t* a, std::format_context& ctx) const {
  if (a->f & flags_t::FLAG_ATOM) return format_atom(a->t,  a->f,array_data(a), ctx.out());
  if (a->t == type_t::T_C8) return format_to(ctx.out(), "\"{}\"", std::string_view(array_data_t_c8(a), a->n));
  std::string out    = "[ ";
  size_t      stride = type_sizeof(a->t, 1);
  const void* ptr    = array_data(a);
  DO(i, a->n) {
    // if (w < c + 4) {
    //   c += fprintf(f, "... ");
    //   break;
    // }
    format_atom(a->t, a->f, ptr + stride * i, std::back_inserter(out));
    out += " ";
  }
  out += "]";

  return std::ranges::copy(out, ctx.out()).out;
}

std::format_context::iterator std::formatter<type_t>::format(const type_t& t, std::format_context& ctx) const {
  return std::ranges::copy(type_name(t), ctx.out()).out;
}