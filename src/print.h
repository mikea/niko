#pragma once

#include <format>
#include <print>
#include <string_view>

#include "array.h"
#include "str.h"

template <>
struct std::formatter<str_t> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const str_t& t, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", std::string_view(t.p, t.l));
  }
};

template <>
struct std::formatter<string_t> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const string_t& t, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "{}", std::string_view(t.p, t.l));
  }
};

template <>
struct std::formatter<type_t> {
  constexpr auto                parse(std::format_parse_context& ctx) { return ctx.begin(); }
  std::format_context::iterator format(const type_t& t, std::format_context& ctx) const;
};

template <>
struct std::formatter<array_p> {
  constexpr auto                parse(std::format_parse_context& ctx) { return ctx.begin(); }
  std::format_context::iterator format(const array_p& a, std::format_context& ctx) const;
};
