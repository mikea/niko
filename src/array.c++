#include "array.h"

array_p array::alloc(type_t t, size_t n) {
  array* a;
  if (data_simd_aligned(t, n)) {
    auto buf = new std::byte[sizeof(array)];
    auto p   = aligned_alloc(SIMD_REG_WIDTH_BYTES, SIMD_ALIGN_BYTES(type_sizeof(t, n)));
    a        = new (buf) array(t, n, nullptr, p);
  } else {
    auto buf = new std::byte[sizeof(array) + type_sizeof(t, n)];
    a        = new (buf) array(t, n, nullptr, buf + sizeof(array));
  }
  if (t == T_ARR) memset(a->p, 0, type_sizeof(t, n));
  return array_p::from_raw(a);
}

array_p array::create(type_t t, size_t n, const void* x) {
  auto a = array::alloc(t, n);
  memcpy(a->p, x, type_sizeof(t, n));
  if (t == T_ARR) DO_MUT_ARRAY(a.get(), arr_t, i, e) e.get()->rc++;
  return a;
}

array_p array::create_slice(array* x, size_t n, const void* p) {
  //   array_inc_ref(x);
  //   array* y = (array*)malloc(sizeof(array));

  //   y->t       = x->t;
  //   y->f       = (flags_t)0;
  //   y->rc      = 1;
  //   y->n       = n;
  //   y->p       = (void*)p;
  //   y->owner   = x;
  //   return array_p::from_raw(y);
  NOT_IMPLEMENTED;
}

array::~array() {
  if (t == T_ARR) DO(i, n) data<arr_t>()[i].~array_p();
  if (simd_aligned()) free(p);
}
