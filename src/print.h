#pragma once

#include <format>

#include "array.h"

// Forward declaration
struct dict_entry;

// Custom formatter for type_t
// Usage: std::format("{}", type) -> "c8", "i64", "f64", "arr", "ffi", or "dict"
template <>
struct std::formatter<type_t> {
  constexpr auto                parse(std::format_parse_context& ctx) { return ctx.begin(); }
  std::format_context::iterator format(const type_t& t, std::format_context& ctx) const;
};

// Custom formatter for array_p
// Format specifiers:
//   - {} or {:}     -> Normal array formatting with brackets: [ 1 2 3 ]
//   - {:n}          -> No brackets, space-separated: 1 2 3
//   - {:10}         -> Width limit (truncates with ...): [ 1 2 ... ]
//   - {:n10}        -> Combined: no brackets with width limit
// Special cases:
//   - Atoms         -> Single value without brackets: 42
//   - Strings (T_C8)-> Quoted format: "hello"
template <>
struct std::formatter<array_p> {
  size_t width    = 0;
  bool   brackets = true;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'n') {
      brackets = false;
      ++it;
    }
    while (it != ctx.end() && *it >= '0' && *it <= '9') width = width * 10 + (*it++ - '0');
    return it;
  }
  std::format_context::iterator format(const array_p& a, std::format_context& ctx) const;
};

// Custom formatter for dict_entry
// Format specifiers:
//   - {}     -> Full definition format (for \d command)
//   - {:s}   -> Short format (just the name)
template <>
struct std::formatter<dict_entry> {
  bool short_format = false;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 's') {
      short_format = true;
      ++it;
    }
    return it;
  }
  std::format_context::iterator format(const dict_entry& e, std::format_context& ctx) const;
};
