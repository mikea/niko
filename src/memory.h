#pragma once

#define ALIGNED(n) ATTR(aligned(n))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)           __ALIGN_MASK(x, (typeof(x))(a)-1)

#define CONSTRUCTOR ATTR(constructor)

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
  inline T&           operator*() { return *t; }
  inline const T&     operator*() const { return *t; }
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