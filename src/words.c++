#include <jemalloc/jemalloc.h>
#include <math.h>
#include <array>
#include <fstream>
#include <sstream>

#include "niko.h"
#include "print.h"
#include "words.h"

#pragma region stack

DEF_WORD("dup", dup) { DUP; }

DEF_WORD("swap", swap) {
  POP(a);
  POP(b);
  PUSH(a);
  PUSH(b);
}

DEF_WORD("drop", drop) { stack.drop(); }

DEF_WORD("nip", nip) {
  POP(a);
  DROP;
  PUSH(a);
}

DEF_WORD("over", over) {
  auto a = stack[1];
  PUSH(a);
}

DEF_WORD("rot", rot) {
  POP(a);
  POP(b);
  POP(c);
  PUSH(b);
  PUSH(a);
  PUSH(c);
}

DEF_WORD("tuck", tuck) {
  POP(a);
  POP(b);
  PUSH(a);
  PUSH(b);
  PUSH(a);
}

DEF_WORD("pick", pick) {
  POP_SIZE(x);
  PUSH(stack[x]);
}

DEF_WORD("2dup", _2dup) {
  PUSH(stack[1]);
  PUSH(stack[1]);
}

DEF_WORD("2swap", _2swap) {
  POP(y2);
  POP(y1);
  POP(x2);
  POP(x1);
  PUSH(y1);
  PUSH(y2);
  PUSH(x1);
  PUSH(x2);
}

DEF_WORD("2drop", _2drop) {
  DROP;
  DROP;
}

DEF_WORD("2over", _2over) {
  PUSH(stack[3]);
  PUSH(stack[3]);
}

#pragma endregion stack

#pragma region bool

ttX X not_impl(X x) { return !x; }
REG_FN11(not, i64_t, not_impl);

DEF_WORD("any", any) {
  POP(x);
  PUSH(array::atom<i64_t>(x->any() ? 1 : 0));
}

DEF_WORD("none", none) {
  POP(x);
  PUSH(array::atom<i64_t>(x->none() ? 1 : 0));
}

DEF_WORD("all", all) {
  POP(x);
  PUSH(array::atom<i64_t>(x->all() ? 1 : 0));
}

#pragma endregion bool

#pragma region conversions

ttX char c8_impl(X x) { return (char)x; }
REG_FN11(c8, c8_t, c8_impl);

// str function to convert numbers to strings
DEF_WORD("str", str) {
  POP(x);

  // Handle single numeric values
  if (x->a && (x->t == T_I64 || x->t == T_F64)) {
    string result;
    if (x->t == T_I64) result = std::to_string(x->data<i64_t>()[0]);
    else result = std::to_string(x->data<f64_t>()[0]);
    PUSH(array::create<c8_t>(result.size(), result.c_str()));
    return;
  }

  // Handle arrays of numbers - return array of strings
  if (x->t == T_I64 || x->t == T_F64) {
    array_p y = array::alloc<arr_t>(x->n);
    for (size_t i = 0; i < x->n; i++) {
      string result;
      if (x->t == T_I64) result = std::to_string(x->data<i64_t>()[i]);
      else result = std::to_string(x->data<f64_t>()[i]);
      y->mut_data<arr_t>()[i] = array::create<c8_t>(result.size(), result.c_str());
    }
    PUSH(y);
    return;
  }

  // For other types, just return as-is (c8 arrays are already strings)
  if (x->t == T_C8) {
    PUSH(x);
    return;
  }

  panicf("cannot convert {} to string", type_name(x->t));
}

// i64 conversion with string parsing support
DEF_WORD("i64", i64) {
  POP(x);

  // Handle string parsing for c8 arrays
  if (x->t == T_C8) {
    string str(x->data<c8_t>(), x->n);
    try {
      i64 result = std::stoll(str);
      PUSH(array::atom<i64_t>(result));
      return;
    } catch (const std::exception&) { panicf("invalid number format: '{}'", str); }
  }

  // Handle other numeric types
  array_p y = x->alloc_as<i64_t>();
  switch (x->t) {
    case T_I64: DO_MUT_ARRAY(y, i64_t, i, dst) { dst = x->data<i64_t>()[i];
      }
      break;
    case T_F64: DO_MUT_ARRAY(y, i64_t, i, dst) { dst = (i64)x->data<f64_t>()[i];
      }
      break;
    default: panicf("cannot convert {} to i64", type_name(x->t));
  }
  PUSH(y);
}

// f64 conversion with string parsing support
DEF_WORD("f64", f64) {
  POP(x);

  // Handle string parsing for c8 arrays
  if (x->t == T_C8) {
    string str(x->data<c8_t>(), x->n);
    try {
      f64 result = std::stod(str);
      PUSH(array::atom<f64_t>(result));
      return;
    } catch (const std::exception&) { panicf("invalid number format: '{}'", str); }
  }

  // Handle other numeric types
  array_p y = x->alloc_as<f64_t>();
  switch (x->t) {
    case T_I64: DO_MUT_ARRAY(y, f64_t, i, dst) { dst = (f64)x->data<i64_t>()[i];
      }
      break;
    case T_F64: DO_MUT_ARRAY(y, f64_t, i, dst) { dst = x->data<f64_t>()[i];
      }
      break;
    default: panicf("cannot convert {} to f64", type_name(x->t));
  }
  PUSH(y);
}

#pragma endregion conversions

#pragma region array_create

DEF_WORD("index", index) {
  POP_SIZE(x);
  array_p y = array::alloc<i64_t>(x);
  DO_MUT_ARRAY(y, i64_t, i, dst) { dst = i; }
  PUSH(y);
}

#pragma endregion array_create

#pragma region array_ops

DEF_WORD("len", len) {
  POP(x);
  PUSH(array::atom<i64_t>(x->n));
}

ttX struct w_reverse {
  static void call(inter_t& inter, stack& stack) {
    POP(x);
    array_p y = x->alloc_as();
    DO(i, x->n) { y->mut_data<X>()[i] = x->data<X>()[x->n - i - 1]; }
    PUSH(y);
  }
};
ffi1_registrar<w_reverse, c8_t, i64_t, f64_t, arr_t> reverse_registrar("reverse");

ttX struct w_split {
  static array_p split_impl(array_p x, array_p y) {
    if (x->t == T_ARR) {
      array_p z = x->alloc_as();
      DO_MUT_ARRAY(z, arr_t, i, dst) { dst = split_impl(x->data<arr_t>()[i], y); }
      return z;
    }

    CHECK(x->t == y->t, "types are not the same: {} vs {}", x->t, y->t);
    assert(y->n == 1);  // not implemented

    stack  stack;
    auto   needle = y->data<X>()[0];
    size_t s      = 0;
    size_t parts  = 0;
    DO_ARRAY(x, X, i, e) {
      if (e == needle) {
        if (i != s) {
          PUSH(array::create<X>(i - s, x->data<X>() + s));
          parts++;
        }
        s = i + 1;
      }
    }
    if (s != x->n) {
      PUSH(array::create<X>(x->n - s, x->data<X>() + s));
      parts++;
    }

    return cat(stack, parts);
  }

  static void call(inter_t& inter, stack& stack) {
    POP(y);
    POP(x);
    PUSH(split_impl(x, y));
  }
};
ffi1_registrar<w_split, c8_t, i64_t, f64_t> split_registrar("split");

ttXY struct w_take {
  static void call(inter_t& inter, stack& stack) {
    POP_SIZE(y);
    POP(x);
    array_p z = array::alloc<X>(y);
    if (x->n == 1) DO(i, y) z->mut_data<X>()[i] = x->data<X>()[0];
    else DO(i, y) z->mut_data<X>()[i] = x->data<X>()[i % x->n];
    PUSH(z);
  }
};
ffi2_registrar<w_take, pair<c8_t, i64_t>, pair<i64_t, i64_t>, pair<f64_t, i64_t>, pair<arr_t, i64_t>> take_registrar(
    "take");

ttXY struct w_cell {
  static array_p cell_impl(array_p x, array_p y) {
    switch (y->t) {
      case T_ARR: {
        array_p z = y->alloc_as();
        DO_MUT_ARRAY(z, arr_t, i, dst) { dst = cell_impl(x, y->data<arr_t>()[i]); }
        return z;
      }
      case T_I64: {
        auto ii = y->data<i64_t>();
        if (y->a && X::e == T_ARR) return x->data<arr_t>()[WRAP(ii[0], x->n)];
        array_p z = y->alloc_as<X>();
        DO_MUT_ARRAY(z, X, i, dst) { dst = x->data<X>()[WRAP(ii[i], x->n)]; }
        return z;
      }
      default: panicf("{} is not supported", y->t);
    }
  }

  static void call(inter_t& inter, stack& stack) {
    POP(y);
    POP(x);
    PUSH(cell_impl(x, y));
  }
};
ffi2_registrar<w_cell,
               pair<c8_t, i64_t>,
               pair<i64_t, i64_t>,
               pair<f64_t, i64_t>,
               pair<arr_t, i64_t>,
               pair<c8_t, arr_t>,
               pair<i64_t, arr_t>,
               pair<f64_t, arr_t>,
               pair<arr_t, arr_t>>
    cell_registrar("[]");

DEF_WORD("cat", cat) {
  POP_SIZE(x);
  PUSH(cat(stack, x));
}

DEF_WORD("tail", tail) {
  POP(x);
  PUSH(x->tail());
}

ttX void repeat(inter_t& inter, stack& stack) {
  POP(y);
  POP(x);
  DO_ARRAY(y, i64_t, i, e) { CHECK(e >= 0, "non-negative values expected"); }
  size_t n = 0;
  DO_ARRAY(y, i64_t, i, e) { n += e; }
  array_p z   = array::alloc<X>(n);
  auto    dst = z->mut_data<X>();
  auto    src = x->data<X>();
  DO_ARRAY(y, i64_t, i, e) {
    DO(j, e) { *dst++ = src[i]; }
  }
  PUSH(z);
}

#define REGISTER_REPEAT(t) repeat_table[t::e][T_I64] = repeat<t>;
CONSTRUCTOR void register_repeat() {
  ffi2_table repeat_table{};
  TYPE_FOREACH(REGISTER_REPEAT);
  global_dict_add_ffi2("repeat", repeat_table);
}

ttX struct w_flip {
  static vector<type_t> guess_types(array_p x) {
    if (x->t != T_ARR) return vector<type_t>(x->n, x->t);
    vector<type_t> v;
    DO_ARRAY(x, arr_t, i, e) { v.push_back(e->a ? e->t : T_ARR); }
    return v;
  }

  static array_p flip_impl(array_p x) {
    if (!x->n) return x;

    size_t         w_max = 0;
    size_t         w_min = size_t_max;
    vector<type_t> types = guess_types(x->data<arr_t>()[0]);
    DO_ARRAY(x, arr_t, i, row) {
      w_max = max(w_max, row->n);
      w_min = min(w_min, row->n);
      while (types.size() < w_max) types.push_back(T_ARR);
      vector<type_t> gt = guess_types(row);
      DO(j, gt.size()) {
        if (gt[j] != types[j]) types[j] = T_ARR;
      }
    }
    assert(w_max == types.size());

    vector<array_p> cols;
    DO(i, w_max) { cols.push_back(array::alloc(types[i], x->n)); }

    DO_ARRAY(x, arr_t, i, row) {
      DO(j, cols.size()) {
        if (j >= row->n) {
          assert(types[j] == T_ARR);
          cols[j]->copy_ij(i, array::atom<arr_t>(array::create<arr_t>(0, nullptr)), 0, 1);
          continue;
        }
        if (types[j] == T_ARR)
          if (row->t == T_ARR) cols[j]->copy_ij(i, row, j, 1);
          else cols[j]->copy_ij(i, array::atom<arr_t>(row->atom_i(j)), 0, 1);
        else if (row->t == T_ARR) cols[j]->copy_ij(i, row->data<arr_t>()[j], 0, 1);
        else cols[j]->copy_ij(i, row, j, 1);
      }
    }

    return array::create<arr_t>(cols.size(), &(*cols.begin()));
  }
  static void call(inter_t& inter, stack& stack) {
    POP(x);
    PUSH(flip_impl(x));
  }
};
ffi1_registrar<w_flip, arr_t> flip_registrar("flip");

#pragma endregion array_ops

#pragma region slash_words

DEF_WORD("\\i", slash_info) {
  printf(VERSION_STRING "\n");
  printf("  %-20s %10ld entries\n", "stack size:", stack.len());
  {
    size_t allocated;
    size_t sz = sizeof(allocated);
    CHECK(mallctl("stats.allocated", &allocated, &sz, NULL, 0) == 0, "failed to query heap size");
    printf("  %-20s %10ld bytes\n", "allocated mem:", allocated);
  }
}

DEF_WORD("\\c", slash_clear) { inter.reset(); }
DEF_WORD("\\mem", slash_mem) { malloc_stats_print(NULL, NULL, NULL); }

DEF_WORD("\\s", slash_stack) {
  DO(i, stack.len()) { (*inter.out) << std::format("{}: {}\n", i, stack[i]); }
}

DEF_WORD("\\d", slash_def) {
  POP_DICT_ENTRY(idx);
  dict_entry& e = inter.dict[idx];
  (*inter.out) << std::format("{}\n", e);
}

#pragma endregion slash_words

#pragma region adverbs

DEF_WORD(",fold", fold) {
  POP_DICT_ENTRY(y);
  POP(x);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(y);
  }
}

DEF_WORD(",scan", scan) {
  POP_DICT_ENTRY(y);
  POP(x);
  DO(i, x->n) {
    if (i > 0) DUP;
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(y);
  }
  PUSH(cat(stack, x->n));
}

DEF_WORD(",apply", apply) {
  POP_DICT_ENTRY(y);
  POP(x);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    inter.entry(y);
  };
  PUSH(cat(stack, x->n));
}

DEF_WORD(",mapply", mapply) {
  POP_DICT_ENTRY(y);
  POP(m);
  POP(x);
  CHECK(m->t == T_I64, "mask: i64 expected");
  CHECK(m->n >= x->n, "mask is not big enough: {} vs {}", m->n, x->n);

  auto md = m->data<i64_t>();
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    if (md[i]) inter.entry(y);
  }
  PUSH(cat(stack, x->n));
}

DEF_WORD(",pairwise", pairwise) {
  POP_DICT_ENTRY(y);
  POP(x);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    if (i > 0) inter.entry(y);
    PUSH(x->atom_i(i));
  };
  DROP;
  PUSH(cat(stack, x->n));
}

DEF_WORD(",power", power) {
  POP_DICT_ENTRY(y);
  POP_SIZE(x);
  DO(i, x) { inter.entry(y); }
}

DEF_WORD(",collect", collect) {
  POP_DICT_ENTRY(y);
  POP_SIZE(x);
  DO(i, x) { inter.entry(y); }
  DROP;
  PUSH(cat(stack, x));
}

DEF_WORD(",trace", trace) {
  POP_DICT_ENTRY(y);
  POP_SIZE(x);
  DO(i, x) {
    if (i > 0) DUP;
    inter.entry(y);
  }
  PUSH(cat(stack, x));
}

DEF_WORD(",while", while_loop) {
  POP_DICT_ENTRY(body);
  for (;;) {
    if (stack.len() == 0 || stack[0]->none()) break;
    inter.entry(body);
  }
}

#pragma endregion adverbs

#pragma region io

DEF_WORD(".", dot) {
  POP(x);
  std::println(*inter.out, "{}", x);
}

DEF_WORD("load_text", load_text) {
  POP(x);
  CHECK(x->t == T_C8, "c8 array expected");
  auto               name = string(x->data<c8_t>(), x->n);
  std::ifstream      file(name);
  std::ostringstream buf;
  buf << file.rdbuf();
  auto content = buf.str();
  PUSH(array::create<c8_t>(content.size(), content.c_str()));
}

#pragma endregion io
