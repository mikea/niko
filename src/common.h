#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <format>
#include <iostream>
#include <ostream>

using std::cerr;
using std::cout;
using std::max;
using std::print;
using std::println;
using std::string;
using str = std::string_view;

#define mv(x) std::move(x)
#define rcast reinterpret_cast

typedef unsigned char byte;
typedef int64_t       i64;
typedef uint64_t      u64;
typedef double        f64;
static_assert(sizeof(f64) == sizeof(i64));

#define restrict __restrict__
#define ttT      template <typename T>
#define ttTtU    template <typename T, typename U>

#define ATTR(attr)    __attribute__((attr))
#define CLEANUP(func) ATTR(cleanup(func))
#define WARN_UNUSED   ATTR(warn_unused_result)
#define DESTRUCTOR    ATTR(destructor)
#define PRINTF(i, j)  ATTR(format(printf, i, j))
#define CONST         ATTR(const)
#define NORETURN      ATTR(noreturn)
#define UNUSED        ATTR(unused)
#define INLINE        static inline
#define ALWAYS_INLINE INLINE ATTR(always_inline)

#define __PASTE__(a, b) a##b
#define PASTE(a, b)     __PASTE__(a, b)

#define UNIQUE(n) PASTE(n, __COUNTER__)

#define NOT_IMPLEMENTED \
  do {                  \
    assert(false);      \
    abort();            \
  } while (0)
#define UNREACHABLE          \
  do {                       \
    __builtin_unreachable(); \
    assert(false);           \
    abort();                 \
  } while (0)

#define assert_this(x) \
  ({                   \
    assert(x);         \
    this;              \
  })

#define _DBG(x) print(cerr, " {} = {}", #x, (x));

#define DBG(...)                                \
  ({                                            \
    do {                                        \
      print(cerr, "{}:{}", __FILE__, __LINE__); \
      APPLY(_DBG, __VA_ARGS__)                  \
      println(cerr);                            \
      cerr.flush();                             \
    } while (0);                                \
  })

#define ERROR(...) println(cerr, __VA_ARGS__)

#define VA_ARGS_FWD(last, call)   \
  ({                              \
    va_list args;                 \
    va_start(args, last);         \
    typeof(call) __result = call; \
    va_end(args);                 \
    __result;                     \
  })

// NARGS

#define NARGS(...)     __NARGS__(__VA_ARGS__, __RSEQ_N__())
#define __NARGS__(...) __NARGS_N__(__VA_ARGS__)

#define __NARGS_N__(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, \
                    _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,  \
                    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59,  \
                    _60, _61, _62, _63, N, ...)                                                                     \
  N

#define __RSEQ_N__()                                                                                                  \
  63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, \
      34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, \
      5, 4, 3, 2, 1, 0

// APPLY

#define __APPLY__1(F, a)                      F(a)
#define __APPLY__2(F, a, b)                   __APPLY__1(F, a) F(b)
#define __APPLY__3(F, a, b, c)                __APPLY__2(F, a, b) F(c)
#define __APPLY__4(F, a, b, c, d)             __APPLY__3(F, a, b, c) F(d)
#define __APPLY__5(F, a, b, c, d, e)          __APPLY__4(F, a, b, c, d) F(e)
#define __APPLY__6(F, a, b, c, d, e, f)       __APPLY__5(F, a, b, c, d, e) F(f)
#define __APPLY__7(F, a, b, c, d, e, f, g)    __APPLY__6(F, a, b, c, d, e, f) F(g)
#define __APPLY__8(F, a, b, c, d, e, f, g, h) __APPLY__7(F, a, b, c, d, e, f, g) F(h)

#define __APPLY__(M, F, ...) M(F, __VA_ARGS__)
#define APPLY(F, ...)        __APPLY__(PASTE(__APPLY__, NARGS(__VA_ARGS__)), F, __VA_ARGS__)

// LOOPS

#define WRAP(i, n)               \
  ({                             \
    typeof(i) __n = n;           \
    ((((i) % __n) + __n) % __n); \
  })

#define _DO(var, n, l) for (size_t var = 0, l = n; var < l; var++)
#define DO(var, n)     _DO(var, n, UNIQUE(__limit))

#define _DOR(var, n, i) for (size_t i = n, var = n - 1; i > 0; i--, var--)
#define DOR(var, n)     _DOR(var, n, UNIQUE(__i))

#define DO_BLOCK(var, l, block) \
  DO(var, l) { block; }

// tools

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// assertions
#define panicf(...) throw std::runtime_error(std::format(__VA_ARGS__))

#define CHECK(cond, ...) \
  if (unlikely(!(cond))) panicf(__VA_ARGS__)

// defer

struct defer_dummy {};
template <class F>
struct deferrer {
  F f;
  ~deferrer() { f(); }
};
template <class F>
deferrer<F> operator*(defer_dummy, F f) {
  return {f};
}
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE)  DEFER_(LINE)
#define defer        auto DEFER(UNIQUE(defer)) = defer_dummy{}* [&]()

#define defer_catch(s)                      \
  defer {                                   \
    if (std::uncaught_exceptions()) { s; }; \
  };
