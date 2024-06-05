#pragma once

#define ALIGNED(n) ATTR(aligned(n))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)           __ALIGN_MASK(x, (typeof(x))(a)-1)

#define CONSTRUCTOR ATTR(constructor)

#define DEF_CLEANUP(t, free_fn)                               \
  typedef t*  own_##t;                                        \
  INLINE void t##_free(void* p) {                             \
    if (p) free_fn((t*)p);                                    \
  }                                                           \
  INLINE void t##_unwind(void* ctx, void* p) { t##_free(p); } \
  INLINE void t##_cleanup(t** p) {                            \
    t##_free(*p);                                             \
    *p = NULL;                                                \
  }                                                           \
  INLINE void t##_cleanup_protected(t** p) {                  \
    t##_free(*p);                                             \
    *p = NULL;                                                \
  }

DEF_CLEANUP(char, free)
DEF_CLEANUP(FILE, fclose)

#define own(t)    CLEANUP(t##_cleanup) own_##t
#define borrow(t) t*

#define protected(t)  CLEANUP(t##_cleanup_protected) t*
#define protect(expr) (expr)

ttT class rc {
  T* t;
  rc(T* t) : t(t) {}
  inline void dec() {
    if (t) t->dec();
  }
  inline void inc() {
    if (t) t->inc();
  }

 public:
  rc() : t(nullptr) {}
  rc(nullptr_t) : t(nullptr) {}
  ~rc() { dec(); }
  rc(const rc& o) : t(o.t) { inc(); }
  rc(rc&& o) : t(o.t) { o.t = nullptr; }
  inline              operator T*() { return t; }          // todo: remove
  inline static rc<T> from_raw(T* t) { return rc<T>(t); }  // todo: remove
  inline T*           operator->() { return t; }
  inline const T*     operator->() const { return t; }
  inline T*           get() { return t; }        // todo: remove
  inline const T*     get() const { return t; }  // todo: remove
  inline bool         operator!() const { return !t; }
  inline void         operator=(nullptr_t) {
    dec();
    t = nullptr;
  }
  inline void operator=(const rc& o) {
    dec();
    t = o.t;
    inc();
  }
};