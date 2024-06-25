#include "words.h"
#include <math.h>

ttX X neg_impl(X x) { return -x; }
REG_FN11(neg, X, neg_impl);

ttX X abs_impl(X x) { return labs(x); }
template <>
f64 abs_impl<f64>(f64 x) {
  return fabs(x);
}
REG_FN11(abs, X, abs_impl);

REG_FN11(acos, f64_t, acos)
REG_FN11(acosh, f64_t, acosh)
REG_FN11(asin, f64_t, asin)
REG_FN11(asinh, f64_t, asinh)
REG_FN11(atan, f64_t, atan)
REG_FN11(atanh, f64_t, atanh)
REG_FN11(cbrt, f64_t, cbrt)
REG_FN11(cos, f64_t, cos)
REG_FN11(cosh, f64_t, cosh)
REG_FN11(erf, f64_t, erf)
REG_FN11(exp, f64_t, exp)
REG_FN11(bessel1_0, f64_t, j0)
REG_FN11(bessel1_1, f64_t, j1)
REG_FN11(bessel2_0, f64_t, y0)
REG_FN11(bessel2_1, f64_t, y1)
REG_FN11(lgamma, f64_t, lgamma)
REG_FN11(log, f64_t, log)
REG_FN11(log10, f64_t, log10)
REG_FN11(log1p, f64_t, log1p)
REG_FN11(log2, f64_t, log2)
REG_FN11(sin, f64_t, sin)
REG_FN11(sinh, f64_t, sinh)
REG_FN11(sqrt, f64_t, sqrt)
REG_FN11(tan, f64_t, tan)
REG_FN11(tanh, f64_t, tanh)
REG_FN11(ceil, i64_t, ceil)
REG_FN11(floor, i64_t, floor)
REG_FN11(round, i64_t, round)
REG_FN11(trunc, i64_t, trunc)
