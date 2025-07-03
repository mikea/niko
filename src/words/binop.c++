#include <math.h>
#include "words.h"

// Utility for computing output type for numeric binops
// Promotes to f64 if either input is f64
// Keeps c8 if both inputs are c8
// Otherwise uses i64
ttXY using numeric_result_t = typename std::conditional<
  std::is_same<X, f64_t>::value || std::is_same<Y, f64_t>::value, 
  f64_t, 
  typename std::conditional<
    std::is_same<X, c8_t>::value && std::is_same<Y, c8_t>::value,
    c8_t,
    i64_t
  >::type
>::type;

// Macro for simple numeric binops that use numeric_result_t type promotion
#define NUMERIC_BINOP(name, symbol, expr) \
  ttXY struct w_##name { \
    using Z = numeric_result_t<X, Y>; \
    static void call(inter_t& inter, stack& stack) { kernel_binop<w_##name, X, Y>(inter, stack); } \
    static inline typename Z::t apply(typename X::t x, typename Y::t y) { return expr; } \
  }; \
  ffi2_registrar<w_##name, \
                 pair<c8_t, c8_t>, pair<c8_t, i64_t>, pair<c8_t, f64_t>, \
                 pair<i64_t, c8_t>, pair<i64_t, i64_t>, pair<i64_t, f64_t>, \
                 pair<f64_t, c8_t>, pair<f64_t, i64_t>, pair<f64_t, f64_t>> \
      name##_registrar(symbol); \
  \
  ttX struct w_##name##_fold { \
    static void call(inter_t& inter, stack& stack) { kernel_fold<w_##name, X>(inter, stack); } \
  }; \
  ffi1_registrar<w_##name##_fold, c8_t, i64_t, f64_t> \
      name##_fold_registrar(symbol ",fold"); \
  \
  ttX struct w_##name##_scan { \
    static void call(inter_t& inter, stack& stack) { kernel_scan<w_##name, X>(inter, stack); } \
  }; \
  ffi1_registrar<w_##name##_scan, c8_t, i64_t, f64_t> \
      name##_scan_registrar(symbol ",scan");

// Macro for comparison binops that always return i64 and support c8_t inputs
#define COMPARISON_BINOP(name, symbol, expr) \
  ttXY struct w_##name { \
    using Z = i64_t; \
    static void call(inter_t& inter, stack& stack) { kernel_binop<w_##name, X, Y>(inter, stack); } \
    static inline i64 apply(typename X::t x, typename Y::t y) { return expr; } \
  }; \
  ffi2_registrar<w_##name, \
                 pair<c8_t, c8_t>, pair<c8_t, i64_t>, pair<c8_t, f64_t>, \
                 pair<i64_t, c8_t>, pair<i64_t, i64_t>, pair<i64_t, f64_t>, \
                 pair<f64_t, c8_t>, pair<f64_t, i64_t>, pair<f64_t, f64_t>> \
      name##_registrar(symbol);

template <template <typename, typename> class Kernel, typename X, typename Y>
array_p kernel_binop(array_p x, array_p y) {
  CHECK(y->n == x->n || y->n == 1 || x->n == 1, "array lengths are incompatible: {} vs {}", x->n, y->n);
  using Z   = Kernel<X, Y>::Z;
  array_p z = array::alloc<Z>(max(x->n, y->n));
  z->a      = x->a & y->a;
  auto xd   = x->data<X>();
  auto yd   = y->data<Y>();
  auto zd   = z->mut_data<Z>();
  if (z->n == x->n && z->n == y->n) DO(i, z->n) zd[i] = Kernel<X, Y>::apply(xd[i], yd[i]);
  else if (x->n == 1 && z->n == y->n) DO(i, z->n) zd[i] = Kernel<X, Y>::apply(xd[0], yd[i]);
  else if (y->n == 1 && z->n == x->n) DO(i, z->n) zd[i] = Kernel<X, Y>::apply(xd[i], yd[0]);
  else DO(i, z->n) zd[i] = Kernel<X, Y>::apply(xd[i % x->n], yd[i % y->n]);
  return z;
}

template <template <typename, typename> class Kernel, typename X, typename Y>
void kernel_binop(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  auto z = kernel_binop<Kernel, X, Y>(x, y);
  PUSH(z);
}

template <template <typename, typename> class Kernel, typename X>
void kernel_fold(inter_t& inter, stack& stack) {
  POP(x);
  if (x->n == 0) {
    PUSH(x);
    return;
  }
  
  using Z = typename Kernel<X, X>::Z;
  
  auto xd = x->data<X>();
  typename Z::t z = xd[0];
  DO1(i, x->n) z = Kernel<X, X>::apply(z, xd[i]);
  PUSH(array::atom<Z>(z));
}

template <template <typename, typename> class Kernel, typename X>
void kernel_scan(inter_t& inter, stack& stack) {
  POP(x);
  if (x->n == 0) {
    PUSH(x);
    return;
  }
  
  using Z = typename Kernel<X, X>::Z;
  
  auto xd = x->data<X>();
  array_p z = array::alloc<Z>(x->n);
  auto zd = z->mut_data<Z>();
  
  zd[0] = xd[0];
  DO1(i, x->n) zd[i] = Kernel<X, X>::apply(zd[i-1], xd[i]);
  PUSH(z);
}

#pragma region divide

ttXY struct w_divide {
  using Z = f64_t;
  static void       call(inter_t& inter, stack& stack) { kernel_binop<w_divide, X, Y>(inter, stack); }
  static inline f64 apply(X::t x, Y::t y) { return (f64)x / (f64)y; }
};
ffi2_registrar<w_divide, pair<i64_t, i64_t>, pair<i64_t, f64_t>, pair<f64_t, i64_t>, pair<f64_t, f64_t>>
    divide_registrar("/");

#pragma endregion divide

#pragma region div

ttXY struct div_impl {
  using Z = f64_t;
  static inline f64 apply(X::t x, Y::t y) { return trunc(x / y); }
};
template <>
struct div_impl<i64_t, i64_t> {
  using Z = i64_t;
  static inline i64 apply(i64 x, i64 y) { return x / y; }
};
ttXY struct w_div {
  static void call(inter_t& inter, stack& stack) { kernel_binop<div_impl, X, Y>(inter, stack); }
};
ffi2_registrar<w_div, pair<i64_t, i64_t>, pair<i64_t, f64_t>, pair<f64_t, i64_t>, pair<f64_t, f64_t>> div_registrar(
    "div");

#pragma endregion div

#pragma region mod

ttXY struct mod_impl {
  using Z = f64_t;
  static inline f64 apply(X::t x, Y::t y) { return fmod(x, y); }
};
template <>
struct mod_impl<i64_t, i64_t> {
  using Z = i64_t;
  static inline i64 apply(i64 x, i64 y) { return x % y; }
};
ttXY struct w_mod {
  static void call(inter_t& inter, stack& stack) { kernel_binop<mod_impl, X, Y>(inter, stack); }
};
ffi2_registrar<w_mod, pair<i64_t, i64_t>, pair<i64_t, f64_t>, pair<f64_t, i64_t>, pair<f64_t, f64_t>> mod_registrar(
    "mod");

#pragma endregion mod

#pragma region equal

ttXY struct w_eq {
  using Z = i64_t;
  static void       call(inter_t& inter, stack& stack) { kernel_binop<w_eq, X, Y>(inter, stack); }
  static inline i64 apply(X::t x, Y::t y) { return x == y; }
};
ttX struct w_eq<X, arr_t> {
  using Z = i64_t;
  static array_p _eq(array_p x, array_p y) {
    CHECK(x->t == y->t, "not implemented");
    return kernel_binop<w_eq, X, X>(x, y);
  }
  static void call(inter_t& inter, stack& stack) {
    POP(y);
    POP(x);
    auto z = y->alloc_as();
    DO_MUT_ARRAY(z, arr_t, i, e) { e = _eq(x, y->data<arr_t>()[i]); }
    PUSH(z);
  }
};
ttY struct w_eq<arr_t, Y> {
  using Z = i64_t;
  static array_p _eq(array_p x, array_p y) {
    CHECK(x->t == y->t, "not implemented");
    return kernel_binop<w_eq, Y, Y>(x, y);
  }
  static void call(inter_t& inter, stack& stack) {
    POP(y);
    POP(x);
    auto z = x->alloc_as();
    DO_MUT_ARRAY(z, arr_t, i, e) { e = _eq(x->data<arr_t>()[i], y); }
    PUSH(z);
  }
};
ffi2_registrar<w_eq,
               pair<c8_t, c8_t>,
               pair<c8_t, i64_t>,
               pair<c8_t, f64_t>,
               pair<c8_t, arr_t>,
               pair<i64_t, c8_t>,
               pair<i64_t, i64_t>,
               pair<i64_t, f64_t>,
               pair<i64_t, arr_t>,
               pair<f64_t, c8_t>,
               pair<f64_t, i64_t>,
               pair<f64_t, f64_t>,
               pair<f64_t, arr_t>,
               pair<arr_t, c8_t>,
               pair<arr_t, i64_t>,
               pair<arr_t, f64_t>>
    eq_registrar("=");

#pragma endregion equal

#pragma region comparisons

COMPARISON_BINOP(less, "<", x < y)
COMPARISON_BINOP(greater, ">", x > y)
COMPARISON_BINOP(less_equal, "<=", x <= y)
COMPARISON_BINOP(greater_equal, ">=", x >= y)
COMPARISON_BINOP(not_equal, "!=", x != y)

#pragma endregion comparisons

#pragma region plus

NUMERIC_BINOP(plus, "+", x + y)

#pragma endregion plus

#pragma region mul

NUMERIC_BINOP(mul, "*", x * y)

#pragma endregion mul

#pragma region minus

NUMERIC_BINOP(minus, "-", x - y)

#pragma endregion minus

#pragma region max

NUMERIC_BINOP(max, "|", x > y ? x : y)

#pragma endregion max

#pragma region min

NUMERIC_BINOP(min, "&", x > y ? y : x)

#pragma endregion min

