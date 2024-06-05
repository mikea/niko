#include "array.h"

array_p array_t::alloc(type_t t, size_t n, flags_t f) {
  array_t* a;
  if (data_simd_aligned(t, n)) {
    auto buf = new std::byte[sizeof(array_t)];
    auto p   = aligned_alloc(SIMD_REG_WIDTH_BYTES, SIMD_ALIGN_BYTES(type_sizeof(t, n)));
    a        = new (buf) array_t(t, f, n, nullptr, p);
  } else {
    auto buf = new std::byte[sizeof(array_t) + type_sizeof(t, n)];
    a        = new (buf) array_t(t, f, n, nullptr, buf + sizeof(array_t));
  }
  if (t == T_ARR) memset(a->p, 0, type_sizeof(t, n));
  return array_p::from_raw(a);
}

array_p array_t::create(type_t t, size_t n, flags_t f, const void* x) {
  auto a = array_t::alloc(t, n, f);
  memcpy(a->p, x, type_sizeof(t, n));
  if (t == T_ARR) DO_MUT_ARRAY(a.get(), t_arr, i, p) p->get()->rc++;
  return a;
}

array_p array_t::create_slice(array_t* x, size_t n, const void* p) {
  //   array_inc_ref(x);
  //   array_t* y = (array_t*)malloc(sizeof(array_t));

  //   y->t       = x->t;
  //   y->f       = (flags_t)0;
  //   y->rc      = 1;
  //   y->n       = n;
  //   y->p       = (void*)p;
  //   y->owner   = x;
  //   return array_p::from_raw(y);
  NOT_IMPLEMENTED;
}

array_t::~array_t() {
  if (t == T_ARR) {
    DO_ARRAY(this, t_arr, i, p) { p->~array_p(); }
  }
  if (simd_aligned()) free(p);
}
