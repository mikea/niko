#include "array.h"
#include "niko.h"

// Undefine CHECK to avoid conflict with Catch2
#ifdef CHECK
#undef CHECK
#endif

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Array creation and basic properties", "[array]") {
    SECTION("Create i64 array") {
        i64 data[] = {1, 2, 3, 4, 5};
        auto arr = array::create<i64_t>(5, data);
        
        REQUIRE(arr->t == T_I64);
        REQUIRE(arr->n == 5);
        REQUIRE(arr->rc == 1);
        REQUIRE(!arr->a); // not atomic
        
        auto arr_data = arr->data<i64_t>();
        for (size_t i = 0; i < 5; i++) {
            REQUIRE(arr_data[i] == data[i]);
        }
    }
    
    SECTION("Create f64 array") {
        f64 data[] = {1.1, 2.2, 3.3};
        auto arr = array::create<f64_t>(3, data);
        
        REQUIRE(arr->t == T_F64);
        REQUIRE(arr->n == 3);
        REQUIRE(!arr->a);
        
        auto arr_data = arr->data<f64_t>();
        REQUIRE(arr_data[0] == 1.1);
        REQUIRE(arr_data[1] == 2.2);
        REQUIRE(arr_data[2] == 3.3);
    }
    
    SECTION("Create c8 array") {
        const char data[] = "hello";
        auto arr = array::create<c8_t>(5, data);
        
        REQUIRE(arr->t == T_C8);
        REQUIRE(arr->n == 5);
        REQUIRE(!arr->a);
        
        auto arr_data = arr->data<c8_t>();
        REQUIRE(strncmp(arr_data, data, 5) == 0);
    }
    
    SECTION("Create empty array") {
        auto arr = array::alloc<i64_t>(0);
        
        REQUIRE(arr->t == T_I64);
        REQUIRE(arr->n == 0);
        REQUIRE(!arr->a);
    }
}

TEST_CASE("Atomic array creation", "[array]") {
    SECTION("i64 atom") {
        i64 value = 42;
        auto arr = array::atom<i64_t>(value);
        
        REQUIRE(arr->t == T_I64);
        REQUIRE(arr->n == 1);
        REQUIRE(arr->a); // is atomic
        REQUIRE(arr->data<i64_t>()[0] == 42);
    }
    
    SECTION("f64 atom") {
        f64 value = 3.14;
        auto arr = array::atom<f64_t>(value);
        
        REQUIRE(arr->t == T_F64);
        REQUIRE(arr->n == 1);
        REQUIRE(arr->a);
        REQUIRE(arr->data<f64_t>()[0] == 3.14);
    }
    
    SECTION("c8 atom") {
        char value = 'x';
        auto arr = array::atom<c8_t>(value);
        
        REQUIRE(arr->t == T_C8);
        REQUIRE(arr->n == 1);
        REQUIRE(arr->a);
        REQUIRE(arr->data<c8_t>()[0] == 'x');
    }
}

TEST_CASE("Array is_true function", "[array]") {
    SECTION("Atomic i64 values") {
        auto true_atom = array::atom<i64_t>(42);
        auto false_atom = array::atom<i64_t>(0);
        
        REQUIRE(true_atom->any());
        REQUIRE(!false_atom->any());
    }
    
    SECTION("Atomic f64 values") {
        auto true_atom = array::atom<f64_t>(3.14);
        auto false_atom = array::atom<f64_t>(0.0);
        
        REQUIRE(true_atom->any());
        REQUIRE(!false_atom->any());
    }
    
    SECTION("Atomic c8 values") {
        auto true_atom = array::atom<c8_t>('x');
        auto false_atom = array::atom<c8_t>(0);
        
        REQUIRE(true_atom->any());
        REQUIRE(!false_atom->any());
    }
    
    SECTION("i64 arrays") {
        i64 all_zeros[] = {0, 0, 0};
        i64 has_nonzero[] = {0, 0, 1};
        i64 all_nonzero[] = {1, 2, 3};
        
        auto false_arr = array::create<i64_t>(3, all_zeros);
        auto true_arr1 = array::create<i64_t>(3, has_nonzero);
        auto true_arr2 = array::create<i64_t>(3, all_nonzero);
        
        REQUIRE(!false_arr->any());
        REQUIRE(true_arr1->any());
        REQUIRE(true_arr2->any());
    }
    
    SECTION("f64 arrays") {
        f64 all_zeros[] = {0.0, 0.0, 0.0};
        f64 has_nonzero[] = {0.0, 0.0, 1.5};
        
        auto false_arr = array::create<f64_t>(3, all_zeros);
        auto true_arr = array::create<f64_t>(3, has_nonzero);
        
        REQUIRE(!false_arr->any());
        REQUIRE(true_arr->any());
    }
    
    SECTION("c8 arrays") {
        char all_nulls[] = {0, 0, 0};
        char has_char[] = {0, 0, 'a'};
        
        auto false_arr = array::create<c8_t>(3, all_nulls);
        auto true_arr = array::create<c8_t>(3, has_char);
        
        REQUIRE(!false_arr->any());
        REQUIRE(true_arr->any());
    }
    
    SECTION("Empty arrays") {
        auto empty_i64 = array::alloc<i64_t>(0);
        auto empty_f64 = array::alloc<f64_t>(0);
        auto empty_c8 = array::alloc<c8_t>(0);
        
        REQUIRE(!empty_i64->any());
        REQUIRE(!empty_f64->any());
        REQUIRE(!empty_c8->any());
    }
    
    SECTION("Nested arrays (array of arrays)") {
        // Create some atomic arrays
        auto false_inner = array::atom<i64_t>(0);
        auto true_inner = array::atom<i64_t>(1);
        
        // Create array containing only false arrays
        array_p false_inners[] = {false_inner, false_inner};
        auto false_outer = array::create<arr_t>(2, false_inners);
        REQUIRE(!false_outer->any());
        
        // Create array containing at least one true array
        array_p mixed_inners[] = {false_inner, true_inner};
        auto true_outer = array::create<arr_t>(2, mixed_inners);
        REQUIRE(true_outer->any());
    }
}

TEST_CASE("Array copying and allocation", "[array]") {
    SECTION("Copy array") {
        i64 data[] = {1, 2, 3};
        auto original = array::create<i64_t>(3, data);
        auto copied = original->copy();
        
        REQUIRE(copied->t == original->t);
        REQUIRE(copied->n == original->n);
        REQUIRE(copied->a == original->a);
        REQUIRE(copied->rc == 1);
        
        // Data should be the same
        auto orig_data = original->data<i64_t>();
        auto copy_data = copied->data<i64_t>();
        for (size_t i = 0; i < 3; i++) {
            REQUIRE(orig_data[i] == copy_data[i]);
        }
    }
    
    SECTION("alloc_as with same type") {
        i64 data[] = {1, 2, 3};
        auto original = array::create<i64_t>(3, data);
        auto new_arr = original->alloc_as();
        
        REQUIRE(new_arr->t == original->t);
        REQUIRE(new_arr->n == original->n);
        REQUIRE(new_arr->a == original->a);
        REQUIRE(new_arr->rc == 1);
    }
    
    SECTION("alloc_as with different type") {
        i64 data[] = {1, 2, 3};
        auto original = array::create<i64_t>(3, data);
        auto new_arr = original->alloc_as<f64_t>();
        
        REQUIRE(new_arr->t == T_F64);
        REQUIRE(new_arr->n == original->n);
        REQUIRE(new_arr->a == original->a);
        REQUIRE(new_arr->rc == 1);
    }
}

TEST_CASE("Array element access", "[array]") {
    SECTION("atom_i for regular arrays") {
        i64 data[] = {10, 20, 30};
        auto arr = array::create<i64_t>(3, data);
        
        auto elem0 = arr->atom_i(0);
        auto elem1 = arr->atom_i(1);
        auto elem2 = arr->atom_i(2);
        
        REQUIRE(elem0->t == T_I64);
        REQUIRE(elem0->a); // should be atomic
        REQUIRE(elem0->data<i64_t>()[0] == 10);
        
        REQUIRE(elem1->data<i64_t>()[0] == 20);
        REQUIRE(elem2->data<i64_t>()[0] == 30);
    }
    
    SECTION("atom_i for array of arrays") {
        auto inner1 = array::atom<i64_t>(100);
        auto inner2 = array::atom<i64_t>(200);
        
        array_p inners[] = {inner1, inner2};
        auto outer = array::create<arr_t>(2, inners);
        
        auto elem0 = outer->atom_i(0);
        auto elem1 = outer->atom_i(1);
        
        REQUIRE(elem0->t == T_I64);
        REQUIRE(elem0->data<i64_t>()[0] == 100);
        REQUIRE(elem1->data<i64_t>()[0] == 200);
    }
    
    SECTION("tail function") {
        i64 data[] = {1, 2, 3, 4, 5};
        auto arr = array::create<i64_t>(5, data);
        auto tail_arr = arr->tail();
        
        REQUIRE(tail_arr->t == T_I64);
        REQUIRE(tail_arr->n == 4); // original size - 1
        
        auto tail_data = tail_arr->data<i64_t>();
        REQUIRE(tail_data[0] == 2);
        REQUIRE(tail_data[1] == 3);
        REQUIRE(tail_data[2] == 4);
        REQUIRE(tail_data[3] == 5);
    }
}

TEST_CASE("Reference counting", "[array]") {
    SECTION("Initial reference count") {
        auto arr = array::atom<i64_t>(42);
        REQUIRE(arr->rc == 1);
    }
    
    SECTION("Reference counting with copies") {
        auto arr1 = array::atom<i64_t>(42);
        REQUIRE(arr1->rc == 1);
        
        auto arr2 = arr1; // Copy the smart pointer
        REQUIRE(arr1->rc == 2);
        REQUIRE(arr2->rc == 2);
    }
}

TEST_CASE("Type checking and validation", "[array]") {
    SECTION("Type assertion") {
        auto i64_arr = array::atom<i64_t>(42);
        auto f64_arr = array::atom<f64_t>(3.14);
        
        // These should not throw
        REQUIRE_NOTHROW(i64_arr->assert_type(T_I64));
        REQUIRE_NOTHROW(f64_arr->assert_type(T_F64));
    }
    
    SECTION("Type assertion with different types") {
        auto i64_arr = array::atom<i64_t>(42);
        auto f64_arr = array::atom<f64_t>(3.14);
        auto c8_arr = array::atom<c8_t>('x');
        
        // These should work
        REQUIRE_NOTHROW(i64_arr->assert_type(T_I64));
        REQUIRE_NOTHROW(f64_arr->assert_type(T_F64));
        REQUIRE_NOTHROW(c8_arr->assert_type(T_C8));
    }
}