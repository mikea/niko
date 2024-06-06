#include <sstream>

#include "inter.h"
#include "print.h"

using std::format_to;

ttT T format_atom(type_t t, bool quote, const void* ptr, T out) {
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
    case T_ARR:        return format_to(out, "{}", *(array_p*)ptr);
    case T_FFI:        NOT_IMPLEMENTED;  // return fprintf(f, "<native_function>");
    case T_DICT_ENTRY: {
      size_t      idx = *(t_dict_entry*)ptr;
      dict_entry& e   = inter_t::current().dict[idx];
      if (quote) return format_to(out, "{}'", e.k);
      else return format_to(out, "{}", e.k);
    }
  }
  UNREACHABLE;
}

std::format_context::iterator std::formatter<array_p>::format(const array_p& a, std::format_context& ctx) const {
  if (a->a) return format_atom(a->t, a->q, a->data(), ctx.out());
  if (a->t == type_t::T_C8) return format_to(ctx.out(), "\"{}\"", str(a->data<c8_t>(), a->n));
  string      out    = "[ ";
  size_t      stride = type_sizeof(a->t, 1);
  const void* ptr    = a->data();
  DO(i, a->n) {
    if (width && width < out.size() + 4) {
      out += "... ";
      break;
    }
    format_atom(a->t, a->q, ptr + stride * i, std::back_inserter(out));
    out += " ";
  }
  out += "]";

  return std::ranges::copy(out, ctx.out()).out;
}

std::format_context::iterator std::formatter<type_t>::format(const type_t& t, std::format_context& ctx) const {
  return std::ranges::copy(type_name(t), ctx.out()).out;
}