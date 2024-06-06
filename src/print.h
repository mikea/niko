#pragma once

#include <string_view>
#include "array.h"

template <>
struct std::formatter<type_t> {
  constexpr auto                parse(std::format_parse_context& ctx) { return ctx.begin(); }
  std::format_context::iterator format(const type_t& t, std::format_context& ctx) const;
};

template <>
struct std::formatter<array_p> {
  size_t width = 0;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    while (it != ctx.end() && *it >= '0' && *it <= '9') width = width * 10 + (*it++ - '0');
    return it;
  }
  std::format_context::iterator format(const array_p& a, std::format_context& ctx) const;
};
