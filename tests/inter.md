# Interpreter and Language Tests

## Input

### atoms: entering and printing

### integers

```nkt
> 1 .
1
> -1 .
-1
> 9223372036854775807 .
9223372036854775807
> 9223372036854775808 .
ERROR: stol
> 0 .
0
```

### floats

```nkt
> 1. .
1.
> 1.0 .
1.
> 9007199254740992. .
9.00719925474099e+15
> 3.1415926 .
3.1415926
> -789.012 .
-789.012
> 1e3
> 1.23e3 .
1230.
> -4.56e-2 .
-0.0456
> 1e30 .
1e+30
> 1e-30 .
1e-30
> 0. .
0.
> 9. .
9.
```

### strings

```nkt
> "" .
""
> "abc" .
"abc"
> "this is test" .
"this is test"
> "abc" len . "this is test" len .
3
12
```

### arrays

```nkt
> [ ] .
[ ]
> [ 1 2 3 ] .
[ 1 2 3 ]
> [ [ 1 2 3 ] [ 4 5 6 ] ] .
[ [ 1 2 3 ] [ 4 5 6 ] ]
> [ [ 1 2 3 ] [ 4 5 6 ] ] len .
2
> [ 1. 2. ] .
[ 1. 2. ]
> [ [ 1. 2. 3. ] [ 4. 5. 6. ] ] len .
2
> [ "abc" "def" ] dup . len .
[ "abc" "def" ]
2
> [ "" "1" "12" "123" ] dup . len .
[ "" "1" "12" "123" ]
4
> [ [ 1. 2. 3. ] [ 4 5 6 ] ] dup . len .
[ [ 1. 2. 3. ] [ 4 5 6 ] ]
2
```

#### error handling

```nkt
> ]
ERROR: unbalanced ]
```

### comments

```nkt
> 1 ( this is comment ) .
1
> 1 (thisiscommenttoo) .
1
> 1 () .
1
```

## Compiler

### `:`

```nkt
> : test_1 1 ; test_1 .
1
> [ 0 1 2 3 4 5 6 7 8 9 ] test_1 + .
[ 1 2 3 4 5 6 7 8 9 10 ]
> : test_1+ test_1 + ; [ 0 1 2 3 4 5 6 7 8 9 ] test_1+ .
[ 1 2 3 4 5 6 7 8 9 10 ]
> [ 0 1 2 3 4 5 6 7 8 9 ] test_1+ .
[ 1 2 3 4 5 6 7 8 9 10 ]
> : 100+ 100 + ; [ 0 1 2 3 4 5 6 7 8 9 ] 100+ .
[ 100 101 102 103 104 105 106 107 108 109 ]
> : centigrade 32 - 5 * 9. / ; 72 centigrade .
22.2222222222222
> : test_sum +' ,fold ;
> [ 0 1 2 3 4 5 6 7 8 9 ] test_sum .
45
> : sums +' ,scan ; 
> [ 0 1 2 3 4 5 6 7 8 9 ] sums .
[ 0 1 3 6 10 15 21 28 36 45 ]
> : test_1. 1. ;
> test_1. .
1.
```

```nkt
> : test_one_one [ 1 1 ] ;
> test_one_one .
[ 1 1 ]
> test_one_one' @ .
[ [ 1 1 ] ]
```

#### redefining words

```nkt
> : 4_2 42 ;
> : 8_4 4_2 2 * ;
> 4_2 . 8_4 .
42
84
> : 4_2 41 ;
> 4_2 . 8_4 .
41
82
> : + 42 ;
ERROR: `+` can't be redefined
> 1 2 + .
3
```

#### error handling

```nkt
> : abc :
ERROR: : can be used only in interpret mode
> 1 . ;
> abc
1
```

### `const`

```nkt
> 42 const forty_two \s
> forty_two .
42
> 76 const forty_two
ERROR: `forty_two` can't be redefined
> .
76
> forty_two .
42
> : forty_two 76 ;
ERROR: `forty_two` can't be redefined
> \s
> forty_two .
42
```

### `var`

```nkt
> [ 1 2 3 ] var one_two_three \s
> one_two_three .
[ 1 2 3 ]
> 123 one_two_three' !
> \s
> one_two_three .
123
```

### `literal`

```nkt
> 5 : plus_five literal + ;
> 3 plus_five .
8
> plus_five' @ .
[ 5 + ]
```

error handling

```nkt
> 5 literal
ERROR: literal can be used only in compilation mode
```

## System Commands

### `\d` (definition lookup)

Test builtin function definitions:

```nkt
> +' \d
+: ffi:[ <c8,c8> <c8,i64> <c8,f64> <i64,c8> <i64,i64> <i64,f64> <f64,c8> <f64,i64> <f64,f64> ]
> .' \d
.: <ffi>
> dup' \d
dup: <ffi>
```

Test user-defined words:

```nkt
> : square dup * ;
> square' \d
: square dup *  ;
```

Test constants:

```nkt
> 42 const answer
> answer' \d
42 const answer
```

Test variables:

```nkt
> 100 var count
> count' \d
var count
```

Test error handling:

```nkt
> 42 \d
ERROR: dict entry expected
```

## Interpreter

### `!`

```nkt
> 3.1 var almost_pi almost_pi .
3.1
> : test_almost_pi almost_pi 2 * . ;
> test_almost_pi
6.2
> 3.14 almost_pi' !
> test_almost_pi
6.28
> : almost_pi 3.141 ;
> test_almost_pi
6.282
> : almost_pi 3.1415 ;
> test_almost_pi
6.283
> 3.1415926 almost_pi' !
> test_almost_pi
6.2831852
```

### `@`

```nkt
> 2 const two
> two .
2
> two' @ .
2
> 3 var three
> three' @ .
3
> 4 three' !
> three' @ .
4
> : five 5 ;
> five' @ .
[ 5 ]
```

## Error Handling

```nkt
> 1999912+
ERROR: unknown word '1999912+'
```

## Word Fusing Optimization

Word fusing allows defining specialized implementations for combinations of quoted words and adverbs (comma words). When executing an adverb with a quoted word on the stack, the interpreter first checks for a fused word with the name "word,adverb".

### Basic Fusing

```nkt
> : test_dup dup ;
> : test_dup,fold drop 999 ;
> [ 1 2 3 ] test_dup' ,fold .
999
```


The fused word `test_dup,fold` is executed instead of the normal `test_dup' ,fold` sequence.

### Multiple Fused Words

```nkt
> : test_neg neg ;
> : test_sqrt sqrt ;  
> : test_plus + ;
> : test_neg,apply drop 777 ;
> : test_sqrt,apply drop 888 ;
> : test_plus,fold drop 555 ;
> [ 1 2 3 ] test_neg' ,apply .
777
> [ 1 4 9 ] test_sqrt' ,apply .
888
> [ 1 2 3 ] test_plus' ,fold .
555
```

### Fallback to Normal Execution

When no fused word is defined, normal execution proceeds:

```nkt
> [ 1 2 3 ] abs' ,apply .
[ 1 2 3 ]
> [ 1 2 3 ] dup +' ,fold .
6
```

### Complex Fused Operations

Fused words can contain any valid Niko code:

```nkt
> : test_double 2 * ;
> : test_double,apply drop 111 ;
> [ 1 2 3 ] test_double' ,apply .
111
> : test_sum,fold drop 222 ;
> [ 1 2 3 4 ] test_sum' ,fold .
222
```

### Fusing with Different Adverbs

```nkt
> : test_inc 1 + ;
> : test_inc,power drop 333 ;
> : test_inc,trace drop 444 ;
> 5 3 test_inc' ,power .
333
> 1 3 test_inc' ,trace .
444
```