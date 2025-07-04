#include <sstream>

#include "inter.h"
#include "print.h"

using std::format_to;

ttT T format_atom(T out, const array& a, size_t i) {
  switch (a.t) {
    case T_I64: return format_to(out, "{}", a.data<i64_t>()[i]);
    case T_F64: {
      char* s = NULL;
      (void)!asprintf(&s, "%.15g", a.data<f64_t>()[i]);
      defer { free(s); };
      if (strchr(s, '.') || strchr(s, 'e')) return format_to(out, "{}", s);
      else return format_to(out, "{}.", s);
    }
    case T_C8:  return format_to(out, "'{}'", a.data<c8_t>()[i]);
    case T_ARR: return format_to(out, "{}", a.data<arr_t>()[i]);
    case T_FFI: {
      ffi fn = a.data<ffi_t>()[i];
      if (!fn) return out;
      if (a.n == T_MAX) return format_to(out, "<{}>", (type_t)i);
      else if (a.n == T_MAX2) return format_to(out, "<{},{}>", (type_t)(i / T_MAX), (type_t)(i % T_MAX));
      return format_to(out, "<ffi>");
    }
    case T_DICT_ENTRY: {
      size_t      idx = a.data<dict_entry_t>()[i];
      dict_entry& e   = inter_t::current().dict[idx];
      if (a.q) return format_to(out, "{:s}'", e);
      else return format_to(out, "{:s}", e);
    }
  }
  UNREACHABLE;
}

std::format_context::iterator std::formatter<array_p>::format(const array_p& a, std::format_context& ctx) const {
  if (a->a) return format_atom(ctx.out(), *a, 0);
  if (a->t == type_t::T_C8) return format_to(ctx.out(), "\"{}\"", str(a->data<c8_t>(), a->n));

  string out = "";
  if (brackets) {
    if (a->t == T_FFI) out += "ffi:";
    out += "[ ";
  }
  DO(i, a->n) {
    if (width && width < out.size() + 4) {
      out += "... ";
      break;
    }
    auto j = out.size();
    format_atom(std::back_inserter(out), *a, i);
    if (j != out.size()) out += " ";
  }
  if (brackets) out += "]";
  return std::ranges::copy(out, ctx.out()).out;
}

std::format_context::iterator std::formatter<type_t>::format(const type_t& t, std::format_context& ctx) const {
  return std::ranges::copy(type_name(t), ctx.out()).out;
}

std::format_context::iterator std::formatter<dict_entry>::format(const dict_entry& e, std::format_context& ctx) const {
  if (short_format) return format_to(ctx.out(), "{}", e.k);
  if (e.sys) return format_to(ctx.out(), "{}: {}", e.k, e.v);
  if (e.cons) return format_to(ctx.out(), "{} const {}", e.v, e.k);
  if (e.var) return format_to(ctx.out(), "var {}", e.k);
  return format_to(ctx.out(), ": {} {:n} ;", e.k, e.v);
}
