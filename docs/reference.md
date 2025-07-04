# Niko Reference

## Defining New Words

`const <NAME> (x ->)` - takes off the top of the stack and defines new word `<NAME>`.
When evaluated `<NAME>` will always result in `x`.

`: <NAME> ... ;`

`var <NAME> (x ->)` - defines new variable with initial value `x`.

`literal (x ->)` - takes the top of the stack and compiles its value as literal into current word definition.

`! (x v' ->)` stores `x` at the address of `v`. Works for `var`/`:` definitions interchangeably.

`@ (v' -> x)` loads value of `v`. Works for `const`/`var`/`:` definitions interchangeably.

## Development Aids

Words that starts from '\' are indended to be used during development
(or debugging). You should rarely need to use them outside of repl.

|Word|Signature|Description|
|---|---|---|
|`\c`|`( ... ->)`|clears the stack|
|`\i`|`( ->)`|prints info about current `niko` status|
|`\mem`|`( ->)`|prints detailed memory usage information|
|`\s`|`( ->)`|prints every stack item|

## Stack Manipulation

|Word|Signature|Description|
|---|---|---|
|`drop`|`(x ->)`|removes top item from the stack|
|`nip`|`(x y -> y)`|drops the second item on the stack|
|`dup`|`(x -> x x)`| duplicates top entry of the stack|
|`over`|`(x y -> x y x)`|places a copy of the second item on top of the stack|
|`rot`|`(x y z -> z x y)`|rotates top three items on the stack|
|`swap`|`(x y -> y x)`|exchanges top to items of the stack|
|`tuck`|`(x y -> y x y)`|places a copy of the top of the stack below the second item|
|`pick`|`( n -> x)`| places a copy of `n`-th element on top of the stack. `0 pick` equals `dup`|
|`2drop`|`(x y ->)`| |
|`2dup`|`(x y -> x y x y)`| |
|`2swap`|`(x1 x2 y1 y2 -> y1 y2 x1 x2)`| |
|`2over`|`(x1 x2 y1 y2 -> x1 x2 y1 y2 x1 x2)`| |

## Creating Arrays

|Word|Signature|Description|
|---|---|---|
|`zeros`|`( s -> x)`|creates array with shape `s` filled with zeros|
|`ones`|`( s -> x)`|creates array with shape `s` filled with ones|
|`index`|`( s -> x)`|creates array with shape `s` filled values from `0` to the length-1|
|`cat`|`(... s -> x)`|creates array of shape `s` out of preceding stack items|

## Array Introspection

|Word|Signature|Description|
|---|---|---|
|`len`|`(x -> n)`|replaces top array with total number of its elements
|`shape`|`(x -> x)`|replaces top array with 1-d array of its shape

## Manipulating Arrays

|Word|Signature|Description|
|---|---|---|
|`tail`|`(x -> y)`|removes one element from x|
|`take`|`(x s -> y)`|takes `x` according to the shape `s` repeating or truncating if necessary|
|`reverse`|`(x -> y)`|reverses all values in the array `x` keeping its shape|
|`repeat`|`(x n -> y)`|repeat cells of `x` according to values of `n`|
|`split`|`(x y -> z)`|splits x using content of y as delimiter|
|`flip`|`(x -> y)`|flips array of rows to array of columns|

## Unary Operations

|Word|Signature|Description|
|---|---|---|
|`abs`|`(x -> y)`|replaces array with new array of the same type and shape with absolute values|
|`neg`|`(x -> y)`|replaces array with new array of the same type and shape with negative values|

### Conversions

|Word|Signature|Description|
|---|---|---|
|`c8`|`(x -> y)`|converts array to c8 type|
|`i64`|`(x -> y)`|converts array to i64 type; parses string numbers (e.g., `"42"` → `42`), errors on invalid format|
|`f64`|`(x -> y)`|converts array to f64 type; parses string numbers (e.g., `"3.14"` → `3.14`), errors on invalid format|
|`str`|`(x -> y)`|converts numbers to string representation (e.g., `42` → `"42"`, `[1 2]` → `["1" "2"]`)|

### Mathematical Functions

|Word|Signature|Description|
|---|---|---|
|`acos`|`(x->y)`|
|`acosh`|`(x->y)`|
|`asin`|`(x->y)`|
|`asinh`|`(x->y)`|
|`atan`|`(x->y)`|
|`atanh`|`(x->y)`|
|`cbrt`|`(x->y)`|
|`ceil`|`(x->y)`|
|`cos`|`(x->y)`|
|`cosh`|`(x->y)`|
|`erf`|`(x->y)`|
|`exp`|`(x->y)`|
|`floor`|`(x->y)`|
|`j0`|`(x->y)`|
|`j1`|`(x->y)`|
|`y0`|`(x->y)`|
|`y1`|`(x->y)`|
|`lgamma`|`(x->y)`|
|`log`|`(x->y)`|
|`log10`|`(x->y)`|
|`log1p`|`(x->y)`|
|`log2`|`(x->y)`|
|`sin`|`(x->y)`|
|`sinh`|`(x->y)`|
|`sqrt`|`(x->y)`|
|`tan`|`(x->y)`|
|`tanh`|`(x->y)`|

## Binary Operations

|Word|Signature|Description|
|---|---|---|
|`+`|`(x y -> z)`| addition|
|`-`|`(x y -> z)`| subtraction|
|`*`|`(x y -> z)`| multiplication|
|`/`|`(x y -> z)`| division|
|`div`|`(x y -> z)`|integer division (quotient)|
|`mod`|`(x y -> z)`|division remainder|
|`&`, `min`|`(x y -> z)`|min|
|`|`,`max` |`(x y -> z)`|max|
|`=`|`(x y -> b)`| equal comparison|
|`<`|`(x y -> b)`| less comparison|

## Forth-style Utilities

|Word|Signature|Description|
|---|---|---|
|`1+`|`(x -> y)`| add 1 (increment)|
|`1-`|`(x -> y)`| subtract 1 (decrement)|
|`2*`|`(x -> y)`| multiply by 2 (double)|
|`2/`|`(x -> y)`| divide by 2 (halve)|
|`2mod`|`(x -> y)`| modulo 2 (even/odd test)|
|`0=`|`(x -> b)`| test if equal to zero|
|`0<>`|`(x -> b)`| test if not equal to zero|
|`any`|`(x -> b)`| true if any element is non-zero|
|`none`|`(x -> b)`| true if no elements are non-zero|
|`all`|`(x -> b)`| true if all elements are non-zero|

## Aggregation

|Word|Signature|Description|
|---|---|---|
|`sum`|`(x -> y)`|sums all elements in `x` keeping only the result
|`sums`|`(x -> y)`|sums all elements in `x` organizing intermediate results in shape of `x`
|`deltas`|`(x -> y)`|computes deltas between consequent elements of `x`, leaving first element unchanged

## Higher Level Words

|Word|Signature|Description|
|---|---|---|
|`,fold`|`(x op' -> y)`| ,fold cells of rank `0` using `op`|
|`,scan`|`(x op' -> y)`| ,fold cells of rank `0` using `op` organizing all intermediate results in `x`'s shape|
|`,apply`|`(x r op' -> y)`| applies `op` to cells of rank `0` organizing results into `x`'s shape|
|`,mapply`|`(x mask op' -> y)`| applies `op` to elements of `x` where `mask` is true, keeps others unchanged|
|`,power`|`(x n op' -> y)`|applies `op` `n` times to `x`|
|`,collect`|`(n op' -> y)`|run `op` `n` times then drop top and cat top `n` stack items|
|`,trace`|`(x s op' -> y)`|applies `op` multiple times to `x` and organizes intermediate results into `s` shape|
|`,pairwise`|`(x op' -> y)`| applies `op` to consequent pais of `x` leaving first element unchanged|
|`,while`|`(op' -> )`|executes `op` repeatedly while the top of stack is truthy|

### Word Fusing

Higher-order words support automatic optimization through **word fusing**. When executing `word' ,adverb`, Niko checks for a specialized word named `word,adverb` and executes it if found.

**Examples:**
- `dup' ,fold` → looks for `dup,fold`
- `neg' ,apply` → looks for `neg,apply`  
- `+' ,scan` → looks for `+,scan`

**Benefits:**
- Zero overhead when unused
- Transparent fallback to normal execution
- Enables specialized optimizations
- Maintains compositional semantics

## Input/Output

|Word|Signature|Description|
|---|---|---|
|`.`|`(x ->)`|printlns x|
|`load_text`|`(x -> y)`|loads file with the name x|

## Constants
|Word|Description|
|---|---|
|`E`|base of natural logarithm
|`LOG2E`|base 2 logarithm of `E`
|`LOG10E`|base 10 logarithm of `E`
|`LN2`|natural logarithm of 2
|`LN10`|natural logarithm of 10 
|`PI`|Pi, the ratio of a circle’s circumference to its diameter
|`PI/2`|`PI`/2
|`PI/4`|`PI`/4
|`1/PI`|1/`PI`
|`2/PI`|2/`PI`
