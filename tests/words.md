# `words.c` tests

## stack manipulation

### dup

```nkt
> 1 dup . .
1
1
```

### drop

```nkt
> 1 2 drop .
1
```

### swap

```nkt
> 1 2 swap . .
1
2
```

### nip

```nkt
> 1 2 nip .
2
```

### over

```nkt
> 1 2 over . . .
1
2
1
```

### rot

```nkt
> 1 2 3 rot . . .
1
3
2
```

### tuck

```nkt
> 1 2 tuck . . .
2
1
2
```

### pick

```nkt
> 0 1 2 3 4 0 pick . 1 pick . 2 pick . 3 pick . 4 pick . \c
4
3
2
1
0
> 1 2 5 pick
ERROR: stack underflow
> \s
0: 5
1: 2
2: 1
```

### `2dup`

```nkt
> 0 1 2dup 4 cat .
[ 0 1 0 1 ]
```

### `2swap`

```nkt
> 0 1 2 3 2swap 4 cat .
[ 2 3 0 1 ]
```

### `2drop`

```nkt
> 0 1 2 3 2drop 2 cat .
[ 0 1 ]
```

### `2over`

```nkt
> 0 1 2 3 2over 6 cat .
[ 0 1 2 3 0 1 ]
```

## creating arrays

### index

```nkt
> 10 index .
[ 0 1 2 3 4 5 6 7 8 9 ]
> [ 10 ] index .
[ 0 1 2 3 4 5 6 7 8 9 ]
```

## information about arrays

### len

```nkt
> 10 index len .
10
> 100 index len .
100
```

## array manipulation

### take

```nkt
> 5 index 9 take .
[ 0 1 2 3 4 0 1 2 3 ]
> 5 index [ 8 ] take .
[ 0 1 2 3 4 0 1 2 ]
> 5 index 3 take .
[ 0 1 2 ]
> 2. 2 take .
[ 2. 2. ]
> "abcdef" 3 take .
"abc"
> [ 1 1. ] 4 take .
[ 1 1. 1 1. ]
```

### reverse

```nkt
> 5 index reverse .
[ 4 3 2 1 0 ]
> [ 1. 2. ] reverse .
[ 2. 1. ]
> "abc" reverse .
"cba"
> [ 1 1. ] reverse .
[ 1. 1 ]
```

### `split`

```nkt
> 5 index 2 split .
[ [ 0 1 ] [ 3 4 ] ]
> [ 0 1 2 0 0 3 4 0 0 0 ] 0 split .
[ [ 1 2 ] [ 3 4 ] ]
> [ [ 1 2 3 4 ] [ 0 1 2 3 4 ] ] 2 split .
[ [ [ 1 ] [ 3 4 ] ] [ [ 0 1 ] [ 3 4 ] ] ]
```

### `[]`

```nkt
> 5 index 0 [] .
0
> 5 index 1 [] .
1
> 5 index 2 [] .
2
> 5 index -11 [] .
4
> 100 index -1 [] .
99
```

picking complicated shapes

```nkt
> 5 index [ 0 2 4 ] [] .
[ 0 2 4 ]
> 5 index [ [ 2 0 ] [ 2 ] 4 ] [] .
[ [ 2 0 ] [ 2 ] 4 ]
```

picking elements of arr

```nkt
> [ [ 1 2 ] [ 3 4 ] ] 0 [] .
[ 1 2 ]
>[ [ 1 2 ] [ 3 4 ] ] [ 0 ] [] .
[ [ 1 2 ] ]
>[ [ 1 2 ] [ 3 4 ] ] [ 1 0 1 ] [] .
[ [ 3 4 ] [ 1 2 ] [ 3 4 ] ]
```

### `cat`

```nkt
> 1 2 3 3 cat .
[ 1 2 3 ]
> [ 1 2 ] [ 3 4 ] 2 cat .
[ [ 1 2 ] [ 3 4 ] ]
> [ 1 2 ] [ 3 4 ] 2 cat 0 [] .
[ 1 2 ]
> [ 1 2 ] [ 3 4 ] 2 cat 0 [] len .
2
> 1 2 3 [ 2 3 ] cat .
ERROR: expected single value
```

### `repeat`

```nkt
> [ 1 2 3 ] [ 0 1 2 ] repeat .
[ 2 3 3 ]
> [ 1. 2. ] [ 2 3 ] repeat .
[ 1. 1. 2. 2. 2. ]
> "abc" [ 3 2 1 ] repeat .
"aaabbc"
> [ [ 1 2 ] [ 3 4 5 ] ] [ 2 2 ] repeat .
[ [ 1 2 ] [ 1 2 ] [ 3 4 5 ] [ 3 4 5 ] ]
```

### `flip`

```nkt
> [ [ 0 1 2 ] [ 3 4 5 ] ] flip .
[ [ 0 3 ] [ 1 4 ] [ 2 5 ] ]
> [ [ 0 1. 2 ] [ 3 4. 5 ] [ 6 7 8 ] [ 9 10 11 "name" ] ] flip .
[ [ 0 3 6 9 ] [ 1. 4. 7 10 ] [ 2 5 8 11 ] [ [ ] [ ] [ ] "name" ] ]
```

```nkt
> [ "abc" "def" ] flip .
[ "ad" "be" "cf" ]
> [ [ "abc" "def" "ghi" ] [ "klm" "nop" "qrs" ] ] flip .
[ [ "abc" "klm" ] [ "def" "nop" ] [ "ghi" "qrs" ] ]
```

### `tail`

```nkt
> 10 index tail .
[ 1 2 3 4 5 6 7 8 9 ]
```

## unary words

### `not`

```nkt
> 1 not .
0
> 100 not .
0
> 0 not .
1
> 0.0 not .
1
> [ 0. 1. 2. 3. ] not .
[ 1 0 0 0 ]
> [ [ 0 1 ] [ 1 0 ] ] not .
[ [ 1 0 ] [ 0 1 ] ]
> [ 0 1. ] not .
[ 1 0 ]
> [ [ [ 0 ] ]  [ [ 1. ] ] ] not .
[ [ [ 1 ] ] [ [ 0 ] ] ]
```

### neg

```nkt
> 1 neg .
-1
> 1. neg .
-1.
> [ 1 1. ] neg .
[ -1 -1. ]
```

### conversions

#### `c8`

```nkt
> [ 50 51 52 ] c8 .
"234"
> 10 c8 .
'
'
```

#### `i64`

```nkt
> 42. i64 .
42
```

#### `f64`

```nkt
> 5 index f64 .
[ 0. 1. 2. 3. 4. ]
```

### Comprehensive Conversion Tests

#### `c8` - Character Conversion

```nkt
> 65 c8 .
'A'
> [ 65 66 67 ] c8 .
"ABC"
> 32 c8 .
' '
> 126 c8 .
'~'
```

#### `i64` - Integer Conversion with String Parsing

Basic numeric conversion:
```nkt
> 42. i64 .
42
> [ 1. 2. 3. ] i64 .
[ 1 2 3 ]
```

String parsing:
```nkt
> "42" i64 .
42
> "-123" i64 .
-123
> "999999999999" i64 .
999999999999
```


Single character parsing:
```nkt
> "5" i64 .
5
> "0" i64 .
0
```

Empty string handling:
```nkt
> "" len .
0
```

#### `f64` - Float Conversion with String Parsing

Basic numeric conversion:
```nkt
> 42 f64 .
42.
> [ 1 2 3 ] f64 .
[ 1. 2. 3. ]
```

String parsing:
```nkt
> "3.14159" f64 .
3.14159
> "-2.5" f64 .
-2.5
> "1e-6" f64 .
1e-06
> "1.23e+10" f64 .
12300000000.
> "0.0" f64 .
0.
```

Single character parsing:
```nkt
> "7" f64 .
7.
> "0" f64 .
0.
```

#### `str` - String Conversion

Integer conversion:
```nkt
> 42 str .
"42"
> -123 str .
"-123"
> 0 str .
"0"
```

Float conversion:
```nkt
> 3.14 str .
"3.140000"
> -2.5 str .
"-2.500000"
> 0. str .
"0.000000"
```

Array conversion:
```nkt
> [ 1 2 3 ] str .
[ "1" "2" "3" ]
> [ 1. 2. ] str .
[ "1.000000" "2.000000" ]
```

String pass-through:
```nkt
> "hello" str .
"hello"
> "" str .
""
```

### Character Code Conversion

Using arithmetic to get character codes:
```nkt
> "abc" 0 + .
[ 97 98 99 ]
> "A" 0 + .
[ 65 ]
> "Z" 0 + .
[ 90 ]
```

Character arithmetic:
```nkt
> "ABC" 32 + .
[ 97 98 99 ]
> "ABC" 32 + c8 .
"abc"
> "9" 0 + 48 - .
[ 9 ]
```

String operations:
```nkt
> "ABC" "   " + .
"abc"
> "hello" 1 * .
[ 104 101 108 108 111 ]
```

### Conversion Error Handling

Valid parsing cases:
```nkt
> "123" i64 .
123
> "12.34" f64 .
12.34
```

Round-trip conversions:
```nkt
> 42 str i64 .
42
> 3.14 str f64 .
3.14
> "hello" str str .
"hello"
```


### abs

```nkt
> [ 1 -1 ] abs .
[ 1 1 ]
> [ 1. -1. ] abs .
[ 1. 1. ]
> [ -1 -1. ] abs .
[ 1 1. ]
```

### sqrt

```nkt
> 2. sqrt .
1.4142135623731
> 2 sqrt .
1.4142135623731
> [ 2. 3 4. ] sqrt .
[ 1.4142135623731 1.73205080756888 2. ]
```

### rounding

```nkt
> 1.5 floor .
1
> 1.5 ceil .
2
> -1.5 floor .
-2
> -1.5 trunc .
-1
> 1.5 round .
2
```

### rest

```nkt 
> 3 index acos .
[ 1.5707963267949 0. nan. ]
> 3 index acosh .
[ -nan. 0. 1.31695789692482 ]
> 3 index asin .
[ 0. 1.5707963267949 nan. ]
> 3 index asinh .
[ 0. 0.881373587019543 1.44363547517881 ]
> 3 index atan .
[ 0. 0.785398163397448 1.10714871779409 ]
> 3 index atanh .
[ 0. inf. -nan. ]
> 3 index cbrt .
[ 0. 1. 1.25992104989487 ]
> 5 index atan ceil .
[ 0. 1. 2. 2. 2. ]
> 3 index cos .
[ 1. 0.54030230586814 -0.416146836547142 ]
> 3 index cosh .
[ 1. 1.54308063481524 3.76219569108363 ]
> 3 index erf .
[ 0. 0.842700792949715 0.995322265018953 ]
> 3 index exp .
[ 1. 2.71828182845905 7.38905609893065 ]
> 3 index bessel1_0 .
[ 1. 0.765197686557967 0.223890779141236 ]
> 3 index bessel1_1 .
[ 0. 0.440050585744933 0.576724807756873 ]
> 3 index bessel2_0 .
[ -inf. 0.088256964215677 0.510375672649745 ]
> 3 index bessel2_1 .
[ -inf. -0.781212821300289 -0.107032431540938 ]
> 3 index lgamma .
[ inf. 0. 0. ]
> 3 index log .
[ -inf. 0. 0.693147180559945 ]
> 3 index log10 .
[ -inf. 0. 0.301029995663981 ]
> 3 index log1p .
[ 0. 0.693147180559945 1.09861228866811 ]
> 3 index sin .
[ 0. 0.841470984807897 0.909297426825682 ]
> 3 index sinh .
[ 0. 1.1752011936438 3.62686040784702 ]
> 3 index sqrt .
[ 0. 1. 1.4142135623731 ]
> 3 index tan .
[ 0. 1.5574077246549 -2.18503986326152 ]
> 3 index tanh .
[ 0. 0.761594155955765 0.964027580075817 ]
```

## binary words

### +

```nkt
> 1 2 + .
3
> 10 index dup + .
[ 0 2 4 6 8 10 12 14 16 18 ]
> 10 index 2 + .
[ 2 3 4 5 6 7 8 9 10 11 ]
> 2 10 index + .
[ 2 3 4 5 6 7 8 9 10 11 ]
> 2. 10 index + .
[ 2. 3. 4. 5. 6. 7. 8. 9. 10. 11. ]
> 10 index 2. + .
[ 2. 3. 4. 5. 6. 7. 8. 9. 10. 11. ]
> 1. 2. + .
3.
> [ 1. 2. ] [ 3. 4. ] + .
[ 4. 6. ]
> [ 2. 3. ] 4. + .
[ 6. 7. ]
> 4. [ 2. 3. ] + .
[ 6. 7. ]
```

comprehensive type tests - all 4 scalar combinations:

```nkt
> 5 7 + .
12
> 5 7. + .
12.
> 5. 7 + .
12.
> 5. 7. + .
12.
```

vector-scalar combinations:

```nkt
> [ 1 2 3 ] 10 + .
[ 11 12 13 ]
> 10 [ 1 2 3 ] + .
[ 11 12 13 ]
> [ 1 2 3 ] 10. + .
[ 11. 12. 13. ]
> 10. [ 1 2 3 ] + .
[ 11. 12. 13. ]
```

vector-vector operations:

```nkt
> [ 1 2 3 ] [ 4 5 6 ] + .
[ 5 7 9 ]
> [ 1. 2. 3. ] [ 4 5 6 ] + .
[ 5. 7. 9. ]
> [ 1 2 3 ] [ 4. 5. 6. ] + .
[ 5. 7. 9. ]
> [ 10 ] [ 1 2 3 ] + .
[ 11 12 13 ]
> [ 1 2 3 ] [ 10 ] + .
[ 11 12 13 ]
```

type conversion:

```nkt
> 1 2. + .
3.
> 1. 2 + .
3.
```

error handling:

```nkt
> 10 index [ 1 1 ] +
ERROR: array lengths are incompatible: 10 vs 2
> \s \c
0: [ 1 1 ]
1: [ 0 1 2 3 4 5 6 7 8 9 ]
> "abc" 2 +
> \s \c
0: [ 99 100 101 ]
```

### *

```nkt
> 312 45 * .
14040
> 10 index dup * .
[ 0 1 4 9 16 25 36 49 64 81 ]
```

comprehensive type tests - all 4 scalar combinations:

```nkt
> 3 4 * .
12
> 3 4. * .
12.
> 3. 4 * .
12.
> 3. 4. * .
12.
```

vector-scalar combinations:

```nkt
> [ 1 2 3 ] 10 * .
[ 10 20 30 ]
> 10 [ 1 2 3 ] * .
[ 10 20 30 ]
> [ 1 2 3 ] 10. * .
[ 10. 20. 30. ]
> 10. [ 1 2 3 ] * .
[ 10. 20. 30. ]
```

vector-vector operations:

```nkt
> [ 2 3 4 ] [ 5 6 7 ] * .
[ 10 18 28 ]
> [ 2. 3. 4. ] [ 5 6 7 ] * .
[ 10. 18. 28. ]
> [ 2 3 4 ] [ 5. 6. 7. ] * .
[ 10. 18. 28. ]
> [ 10 ] [ 1 2 3 ] * .
[ 10 20 30 ]
> [ 1 2 3 ] [ 10 ] * .
[ 10 20 30 ]
```

type conversion:

```nkt
> 2 3. * .
6.
> 2. 3 * .
6.
```

error handling:

```nkt
> [ 1 2 3 ] [ 4 5 ] *
ERROR: array lengths are incompatible: 3 vs 2
> \s \c
0: [ 4 5 ]
1: [ 1 2 3 ]
> "abc" 2 *
> \s \c
0: [ 194 196 198 ]
```

### -

```nkt
> 10 index 1 10 take - .
[ -1 0 1 2 3 4 5 6 7 8 ]
```

comprehensive type tests - all 4 scalar combinations:

```nkt
> 10 3 - .
7
> 10 3. - .
7.
> 10. 3 - .
7.
> 10. 3. - .
7.
```

vector-scalar combinations:

```nkt
> [ 10 20 30 ] 5 - .
[ 5 15 25 ]
> 100 [ 10 20 30 ] - .
[ 90 80 70 ]
> [ 10 20 30 ] 5. - .
[ 5. 15. 25. ]
> 100. [ 10 20 30 ] - .
[ 90. 80. 70. ]
```

vector-vector operations:

```nkt
> [ 10 20 30 ] [ 1 2 3 ] - .
[ 9 18 27 ]
> [ 10. 20. 30. ] [ 1 2 3 ] - .
[ 9. 18. 27. ]
> [ 10 20 30 ] [ 1. 2. 3. ] - .
[ 9. 18. 27. ]
> [ 100 ] [ 10 20 30 ] - .
[ 90 80 70 ]
> [ 10 20 30 ] [ 5 ] - .
[ 5 15 25 ]
```

type conversion:

```nkt
> 5 2. - .
3.
> 5. 2 - .
3.
```

error handling:

```nkt
> [ 1 2 3 ] [ 4 5 ] -
ERROR: array lengths are incompatible: 3 vs 2
> \s \c
0: [ 4 5 ]
1: [ 1 2 3 ]
> "abc" 2 -
> \s \c
0: [ 95 96 97 ]
```

### /

```nkt
> 1 2 / .
0.5
> 10 index 1. + dup / .
[ 1. 1. 1. 1. 1. 1. 1. 1. 1. 1. ]
> 1. 2. / .
0.5
> [ 1. ] 2. / .
[ 0.5 ]
> 1. [ 2. ] / .
[ 0.5 ]
> 1 2. / .
0.5
> 1. 2 / .
0.5
```

### div

Integer division (returns integer quotient for integer inputs):

```nkt
> 7 3 div .
2
> 10 3 div .
3
> 15 4 div .
3
> 20 6 div .
3
```

Mixed type division (truncated):

```nkt
> 7 3. div .
2.
> 7. 3 div .
2.
> 7. 3. div .
2.
> 7.8 3.2 div .
2.
> 9.7 2.1 div .
4.
> -7.8 3.2 div .
-2.
```

Vector operations:

```nkt
> [ 7 10 15 20 ] 3 div .
[ 2 3 5 6 ]
> [ 7 10 15 20 ] [ 2 3 4 5 ] div .
[ 3 3 3 4 ]
> 21 [ 2 3 4 5 ] div .
[ 10 7 5 4 ]
```

### `=`

```nkt
> 1 2 = .
0
> 1 1 = .
1
> 2 index 1 = .
[ 0 1 ]
```

cells:

```nkt
> 6 index 1 = .
[ 0 1 0 0 0 0 ]
> 6 index [ 3 3 3 3 3 3 ] = .
[ 0 0 0 1 0 0 ]
> 1 6 index = .
[ 0 1 0 0 0 0 ]
> [ 2 2 ] [ [ 0 1 ] [ 2 3 ] [ 4 5 ] ] = .
[ [ 0 0 ] [ 1 0 ] [ 0 0 ] ]
> [ [ 0 1 ] [ 2 3 ] [ 4 5 ] ] [ 2 2 ] = .
[ [ 0 0 ] [ 1 0 ] [ 0 0 ] ]
```

types:

```nkt
> "abc" "cba" = .
[ 0 1 0 ]
> "abcdef" 100 = .
[ 0 0 0 1 0 0 ]
> "abcdef" 100. = .
[ 0 0 0 1 0 0 ]
> 98 "abc" = .
[ 0 1 0 ]
> 3 index 1 = .
[ 0 1 0 ]
> 3 index 1. = .
[ 0 1 0 ]
> 98. "abc" = .
[ 0 1 0 ]
> 3 index 0. + 1 = .
[ 0 1 0 ]
> 3 index 0. + 1. = .
[ 0 1 0 ]
```

lists

```
>  [ 0 1. ] 1 = .
```

### `<`

```nkt
> 1 2. < .
1
> [ 0 1 2 3 ] 2 < .
[ 1 1 0 0 ]
```

comprehensive comparison tests - all operators, all 4 scalar combinations:

```nkt
> 3 5 < .
1
> 3 5. < .
1
> 3. 5 < .
1
> 3. 5. < .
1
> 5 3 < .
0
> 3 5 > .
0
> 3 5. > .
0
> 3. 5 > .
0
> 3. 5. > .
0
> 5 3 > .
1
> 3 5 <= .
1
> 3 5. <= .
1
> 3. 5 <= .
1
> 3. 5. <= .
1
> 5 5 <= .
1
> 5 3 <= .
0
> 3 5 >= .
0
> 3 5. >= .
0
> 3. 5 >= .
0
> 3. 5. >= .
0
> 5 5 >= .
1
> 5 3 >= .
1
> 3 5 != .
1
> 3 5. != .
1
> 3. 5 != .
1
> 3. 5. != .
1
> 5 5 != .
0
> 5. 5. != .
0
```

vector-scalar combinations:

```nkt
> [ 1 3 5 ] 3 < .
[ 1 0 0 ]
> [ 1 3 5 ] 3 > .
[ 0 0 1 ]
> [ 1 3 5 ] 3 <= .
[ 1 1 0 ]
> [ 1 3 5 ] 3 >= .
[ 0 1 1 ]
> [ 1 3 5 ] 3 != .
[ 1 0 1 ]
> 3 [ 1 3 5 ] < .
[ 0 0 1 ]
> 3 [ 1 3 5 ] > .
[ 1 0 0 ]
> 3 [ 1 3 5 ] <= .
[ 0 1 1 ]
> 3 [ 1 3 5 ] >= .
[ 1 1 0 ]
> 3 [ 1 3 5 ] != .
[ 1 0 1 ]
```

vector-vector operations:

```nkt
> [ 1 3 5 ] [ 2 3 4 ] < .
[ 1 0 0 ]
> [ 1 3 5 ] [ 2 3 4 ] > .
[ 0 0 1 ]
> [ 1 3 5 ] [ 2 3 4 ] <= .
[ 1 1 0 ]
> [ 1 3 5 ] [ 2 3 4 ] >= .
[ 0 1 1 ]
> [ 1 3 5 ] [ 2 3 4 ] != .
[ 1 0 1 ]
> [ 10 ] [ 1 3 5 ] < .
[ 0 0 0 ]
> [ 1 3 5 ] [ 3 ] < .
[ 1 0 0 ]
```

mixed type comparisons:

```nkt
> [ 1 3 5 ] 3. < .
[ 1 0 0 ]
> [ 1. 3. 5. ] 3 > .
[ 0 0 1 ]
> [ 1 3 5 ] [ 2. 3. 4. ] <= .
[ 1 1 0 ]
```

character comparisons:

```nkt
> "abc" "bcd" < .
[ 1 1 1 ]
> "abc" 98 < .
[ 1 0 0 ]
> 98 "abc" > .
[ 1 0 0 ]
```

error handling:

```nkt
> [ 1 2 3 ] [ 4 5 ] <
ERROR: array lengths are incompatible: 3 vs 2
> \s \c
0: [ 4 5 ]
1: [ 1 2 3 ]
```

### max

```nkt
> 10 index 5 | .
[ 5 5 5 5 5 5 6 7 8 9 ]
> 10 index dup 5 = swap 6 = | .
[ 0 0 0 0 0 1 1 0 0 0 ]
```

comprehensive type tests - all 4 scalar combinations:

```nkt
> 3 7 | .
7
> 3 7. | .
7.
> 3. 7 | .
7.
> 3. 7. | .
7.
```

vector-scalar combinations:

```nkt
> [ 1 5 3 ] 4 | .
[ 4 5 4 ]
> 4 [ 1 5 3 ] | .
[ 4 5 4 ]
> [ 1 5 3 ] 4. | .
[ 4. 5. 4. ]
> 4. [ 1 5 3 ] | .
[ 4. 5. 4. ]
```

vector-vector operations:

```nkt
> [ 1 5 3 ] [ 2 4 6 ] | .
[ 2 5 6 ]
> [ 1. 5. 3. ] [ 2 4 6 ] | .
[ 2. 5. 6. ]
> [ 1 5 3 ] [ 2. 4. 6. ] | .
[ 2. 5. 6. ]
> [ 10 ] [ 1 5 3 ] | .
[ 10 10 10 ]
> [ 1 5 3 ] [ 4 ] | .
[ 4 5 4 ]
```

type conversion:

```nkt
> 3 7. | .
7.
> 3. 7 | .
7.
```

error handling:

```nkt
> [ 1 2 3 ] [ 4 5 ] |
ERROR: array lengths are incompatible: 3 vs 2
> \s \c
0: [ 4 5 ]
1: [ 1 2 3 ]
> "abc" 2 |
> \s \c
0: [ 97 98 99 ]
```

### min

```nkt
> 10 index 5 & .
[ 0 1 2 3 4 5 5 5 5 5 ]
```

comprehensive type tests - all 4 scalar combinations:

```nkt
> 3 7 & .
3
> 3 7. & .
3.
> 3. 7 & .
3.
> 3. 7. & .
3.
```

vector-scalar combinations:

```nkt
> [ 1 5 3 ] 4 & .
[ 1 4 3 ]
> 4 [ 1 5 3 ] & .
[ 1 4 3 ]
> [ 1 5 3 ] 4. & .
[ 1. 4. 3. ]
> 4. [ 1 5 3 ] & .
[ 1. 4. 3. ]
```

vector-vector operations:

```nkt
> [ 1 5 3 ] [ 2 4 6 ] & .
[ 1 4 3 ]
> [ 1. 5. 3. ] [ 2 4 6 ] & .
[ 1. 4. 3. ]
> [ 1 5 3 ] [ 2. 4. 6. ] & .
[ 1. 4. 3. ]
> [ 10 ] [ 1 5 3 ] & .
[ 1 5 3 ]
> [ 1 5 3 ] [ 4 ] & .
[ 1 4 3 ]
```

type conversion:

```nkt
> 3 7. & .
3.
> 3. 7 & .
3.
```

error handling:

```nkt
> [ 1 2 3 ] [ 4 5 ] &
ERROR: array lengths are incompatible: 3 vs 2
> \s \c
0: [ 4 5 ]
1: [ 1 2 3 ]
> "abc" 2 &
> \s \c
0: [ 2 2 2 ]
```

### div

```nkt
> 10 3 div .
3
> 10. 3. div .
3.
```

### mod

```nkt
> 10 3 mod .
1
> -10 3 mod .
-1
> 10. 3. mod .
1.
> 10 3. mod .
1.
> 10. 3 mod .
1.
> 10.5 3 mod .
1.5
> 7.8 2.3 mod .
0.9
> 9.7 2.1 mod .
1.3
> -7.8 2.3 mod .
-0.9
```

## higher order words

### ,fold

```nkt
> 24 index +' ,fold .
276
```

### Fused Fold Operations

Native fused implementations for math binary operators provide optimized performance:

#### Basic Operations

```nkt
> [ 1 2 3 4 5 ] +,fold .
15
> [ 2 3 4 ] *,fold .
24
> [ 5 2 8 1 9 ] |,fold .
9
> [ 5 2 8 1 9 ] &,fold .
1
> [ 10 3 7 ] -,fold .
0
```

#### Empty Arrays

Empty arrays return empty arrays:

```nkt
> 42 [ ] +,fold .
[ ]
> 99 [ ] *,fold .
[ ]
> 77 [ ] |,fold .
[ ]
> 55 [ ] &,fold .
[ ]
> 33 [ ] -,fold .
[ ]
```

#### Single Element Arrays

Single element arrays return the element:

```nkt
> [ 42 ] +,fold .
42
> [ 99 ] *,fold .
99
> [ 77 ] |,fold .
77
> [ 55 ] &,fold .
55
> [ 33 ] -,fold .
33
```

#### Float Arrays

```nkt
> [ 1.5 2.5 3.5 ] +,fold .
7.5
> [ 2. 3. 4. ] *,fold .
24.
> [ 5.1 2.2 8.8 1.1 9.9 ] |,fold .
9.9
> [ 5.1 2.2 8.8 1.1 9.9 ] &,fold .
1.1
```

#### Character Arrays

Character arrays (c8) return character results:

```nkt
> "abc" +,fold .
'&'
> "AB" *,fold 0 + .
-62
> "hello" |,fold .
'o'
> "hello" &,fold .
'e'
```

#### Array of Arrays

Fused operations do not support arrays:

```nkt
> [ [ 1 2 ] [ 3 4 ] [ 5 6 ] ] +,fold .
ERROR: '+,fold' does not support arr
> [ [ 1 2 ] [ 3 4 ] ] *,fold .
ERROR: '*,fold' does not support arr
> [ [ 5 2 ] [ 8 1 ] [ 3 9 ] ] |,fold .
ERROR: '|,fold' does not support arr
> [ [ 5 2 ] [ 8 1 ] [ 3 9 ] ] &,fold .
ERROR: '&,fold' does not support arr
```

But unfused operations work with arrays:

```nkt
> [ [ 1 2 ] [ 3 4 ] [ 5 6 ] ] +' ,fold .
[ 9 12 ]
> [ [ 1 2 ] [ 3 4 ] ] *' ,fold .
[ 3 8 ]
> [ [ 5 2 ] [ 8 1 ] [ 3 9 ] ] |' ,fold .
[ 8 9 ]
> [ [ 5 2 ] [ 8 1 ] [ 3 9 ] ] &' ,fold .
[ 3 1 ]
```

#### Equivalence with Generic Fold

Fused operations produce identical results to generic fold:

```nkt
> [ 1 2 3 4 5 ] +' ,fold . [ 1 2 3 4 5 ] +,fold .
15
15
> [ 5 2 8 1 9 ] |' ,fold . [ 5 2 8 1 9 ] |,fold .
9
9
```

#### Large Arrays

Fused operations handle large arrays efficiently:

```nkt
> 1000 index +,fold .
499500
> 10 index 1 + *,fold .
3628800
```

#### Negative Numbers

```nkt
> [ -5 -2 -8 -1 -9 ] |,fold .
-1
> [ -5 -2 -8 -1 -9 ] &,fold .
-9
> [ 10 -3 -7 ] -,fold .
20
```

#### Mixed Signs

```nkt
> [ -5 2 -8 1 9 ] +,fold .
-1
> [ -2 3 -4 ] *,fold .
24
```

### ,scan

```nkt
> 24 index +' ,scan .
[ 0 1 3 6 10 15 21 28 36 45 55 66 78 91 105 120 136 153 171 190 210 231 253 276 ]
```

### fused ,scan operations

```nkt
> [ 1 2 3 4 5 ] +,scan .
[ 1 3 6 10 15 ]
> [ 2 3 4 ] *,scan .
[ 2 6 24 ]
> [ 5 2 8 1 9 ] &,scan .
[ 5 2 2 1 1 ]
> [ 5 2 8 1 9 ] |,scan .
[ 5 5 8 8 9 ]
> [ 10 3 7 ] -,scan .
[ 10 7 0 ]
> 5 index +,scan .
[ 0 1 3 6 10 ]
> [ 1 1 1 1 1 1 1 1 1 1 ] +,scan .
[ 1 2 3 4 5 6 7 8 9 10 ]
```

### apply

```nkt
> 5 index index' ,apply .
[ [ ] [ 0 ] [ 0 1 ] [ 0 1 2 ] [ 0 1 2 3 ] ]
> : head 0 [] ;
> [ [ 1 1. ] [ 2 2. ] ] head' ,apply .
[ 1 2 ]
```

### ,pairwise

```nkt
> : -neg - neg ;
> [ 0 1 1 0 1 0 ] -neg' ,pairwise . \s
[ 0 1 0 -1 1 -1 ]
> 24 index -neg' ,pairwise . \s
[ 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 ]
```

### ,power

```nk
: 2* 2 * ;
```

```nkt
> 1 10 2*' ,power .
1024
> [ 1 2 ] 10 2*' ,power .
[ 1024 2048 ]
```

### `,collect`

```nkt
> : d2* dup 2 * ;
> 1 10 d2*' ,collect .
[ 1 2 4 8 16 32 64 128 256 512 ]
```

### ,trace

```nkt
> 1 10 2*' ,trace .
[ 2 4 8 16 32 64 128 256 512 1024 ]
```

## error handling

```nkt
> 1999912+
ERROR: unknown word '1999912+'
```

## \words

### \c

```nkt
> 10 index \c .
ERROR: stack underflow
```

### \s

```nkt
> \s
> 1 2 3 \s
0: 3
1: 2
2: 1
```

## IO

### `.`

```nkt
> 1000 index .
[ 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 239 240 241 242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270 271 272 273 274 275 276 277 278 279 280 281 282 283 284 285 286 287 288 289 290 291 292 293 294 295 296 297 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314 315 316 317 318 319 320 321 322 323 324 325 326 327 328 329 330 331 332 333 334 335 336 337 338 339 340 341 342 343 344 345 346 347 348 349 350 351 352 353 354 355 356 357 358 359 360 361 362 363 364 365 366 367 368 369 370 371 372 373 374 375 376 377 378 379 380 381 382 383 384 385 386 387 388 389 390 391 392 393 394 395 396 397 398 399 400 401 402 403 404 405 406 407 408 409 410 411 412 413 414 415 416 417 418 419 420 421 422 423 424 425 426 427 428 429 430 431 432 433 434 435 436 437 438 439 440 441 442 443 444 445 446 447 448 449 450 451 452 453 454 455 456 457 458 459 460 461 462 463 464 465 466 467 468 469 470 471 472 473 474 475 476 477 478 479 480 481 482 483 484 485 486 487 488 489 490 491 492 493 494 495 496 497 498 499 500 501 502 503 504 505 506 507 508 509 510 511 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533 534 535 536 537 538 539 540 541 542 543 544 545 546 547 548 549 550 551 552 553 554 555 556 557 558 559 560 561 562 563 564 565 566 567 568 569 570 571 572 573 574 575 576 577 578 579 580 581 582 583 584 585 586 587 588 589 590 591 592 593 594 595 596 597 598 599 600 601 602 603 604 605 606 607 608 609 610 611 612 613 614 615 616 617 618 619 620 621 622 623 624 625 626 627 628 629 630 631 632 633 634 635 636 637 638 639 640 641 642 643 644 645 646 647 648 649 650 651 652 653 654 655 656 657 658 659 660 661 662 663 664 665 666 667 668 669 670 671 672 673 674 675 676 677 678 679 680 681 682 683 684 685 686 687 688 689 690 691 692 693 694 695 696 697 698 699 700 701 702 703 704 705 706 707 708 709 710 711 712 713 714 715 716 717 718 719 720 721 722 723 724 725 726 727 728 729 730 731 732 733 734 735 736 737 738 739 740 741 742 743 744 745 746 747 748 749 750 751 752 753 754 755 756 757 758 759 760 761 762 763 764 765 766 767 768 769 770 771 772 773 774 775 776 777 778 779 780 781 782 783 784 785 786 787 788 789 790 791 792 793 794 795 796 797 798 799 800 801 802 803 804 805 806 807 808 809 810 811 812 813 814 815 816 817 818 819 820 821 822 823 824 825 826 827 828 829 830 831 832 833 834 835 836 837 838 839 840 841 842 843 844 845 846 847 848 849 850 851 852 853 854 855 856 857 858 859 860 861 862 863 864 865 866 867 868 869 870 871 872 873 874 875 876 877 878 879 880 881 882 883 884 885 886 887 888 889 890 891 892 893 894 895 896 897 898 899 900 901 902 903 904 905 906 907 908 909 910 911 912 913 914 915 916 917 918 919 920 921 922 923 924 925 926 927 928 929 930 931 932 933 934 935 936 937 938 939 940 941 942 943 944 945 946 947 948 949 950 951 952 953 954 955 956 957 958 959 960 961 962 963 964 965 966 967 968 969 970 971 972 973 974 975 976 977 978 979 980 981 982 983 984 985 986 987 988 989 990 991 992 993 994 995 996 997 998 999 ]
> 1 2 3 . . .
3
2
1
```

### load_text

```nkt
> "tests/a.txt" load_text .
"a"
> "tests/b.txt" load_text .
"b
b
"
```

## ,while adverb tests

Basic countdown:
```nkt
> : countdown dup . 1 - ; 5 countdown' ,while .
5
4
3
2
1
0
```

GCD algorithm (simplified):
```nkt
> : gcd-step over over mod rot drop ; 12 8 gcd-step' ,while drop .
4
```

Simple countdown starting from 3:
```nkt
> : decr dup . 1 - ; 3 decr' ,while .
3
2
1
0
```

Collatz conjecture steps:
```nkt
> : collatz-step dup . dup 2 mod 0 = 3 * 1 + 2 / | 1 - ; 7 collatz-step' ,while .
7
6.
5.
4.
3.
2.
1.
0.
```

## `,mapply` tests

Basic functionality with simple operations:
```nkt
> : 2* 2 * ; [ 1 2 3 4 ] [ 1 0 1 0 ] 2*' ,mapply .
[ 2 2 6 4 ]
```

All-false mask (should keep original):
```nkt
> [ 10 20 30 ] [ 0 0 0 ] neg' ,mapply .
[ 10 20 30 ]
```

All-true mask (should apply to all):
```nkt
> : square dup * ; [ 2 3 4 ] [ 1 1 1 ] square' ,mapply .
[ 4 9 16 ]
```

Mixed mask with different operation:
```nkt
> : 2/ 2 / ; [ 10 20 30 40 ] [ 1 0 0 1 ] 2/' ,mapply .
[ 5. 20 30 20. ]
```

Single element arrays:
```nkt
> : triple 3 * ; [ 5 ] [ 1 ] triple' ,mapply .
[ 15 ]
> : triple 3 * ; [ 5 ] [ 0 ] triple' ,mapply .
[ 5 ]
```

Complex operations:
```nkt
> : add-ten 10 + ; [ 1 2 3 4 5 ] [ 0 1 0 1 0 ] add-ten' ,mapply .
[ 1 12 3 14 5 ]
```

Works with floating point:
```nkt
> : sqrt-op sqrt ; [ 4. 9. 16. ] [ 1 0 1 ] sqrt-op' ,mapply .
[ 2. 9. 4. ]
```

Empty arrays:
```nkt
> : id ; [ ] [ ] id' ,mapply .
[ ]
```

Operations that change types:
```nkt
> : to-float f64 ; [ 1 2 3 ] [ 1 0 1 ] to-float' ,mapply .
[ 1. 2 3. ]
```

Complex stack operations:
```nkt
> : dup-add dup + ; [ 5 10 15 ] [ 1 0 1 ] dup-add' ,mapply .
[ 10 10 30 ]
```

Verify mask must be i64 array and same length:
```nkt
> : 2* 2 * ; [ 1 2 3 ] [ 1 1 ] 2*' ,mapply .
ERROR: mask is not big enough: 2 vs 3
```

Large arrays work correctly:
```nkt
> : 1+ 1 + ; [ 1 2 3 4 5 6 7 8 ] [ 1 0 1 0 1 0 1 0 ] 1+' ,mapply .
[ 2 2 4 4 6 6 8 8 ]
```

Alternating pattern:
```nkt
> : neg-one -1 * ; [ 1 2 3 4 5 6 ] [ 1 0 1 0 1 0 ] neg-one' ,mapply .
[ -1 2 -3 4 -5 6 ]
```

## Boolean testing words

### any
```nkt
> [ 0 0 1 ] any .
1
> [ 0 0 0 ] any .
0
> [ 1 2 3 ] any .
1
> [ ] any .
0
> 5 any .
1
> 0 any .
0
```

### none  
```nkt
> [ 0 0 0 ] none .
1
> [ 0 0 1 ] none .
0
> [ 1 2 3 ] none .
0
> [ ] none .
1
> 0 none .
1
> 5 none .
0
```

### all
```nkt
> [ 1 2 3 ] all .
1
> [ 1 0 3 ] all .
0
> [ 0 0 0 ] all .
0
> [ ] all .
1
> 5 all .
1
> 0 all .
0
```

Floating point arrays:
```nkt
> [ 0. 1. 2. ] any .
1
> [ 0. 0. 0. ] any .
0
> [ 1. 2. 3. ] all .
1
> [ 1. 0. 3. ] all .
0
```
