# Niko Reference

## Defining New Words

`const <NAME> (x -> )` - takes off the top of the stack and defines new word `<NAME>`.
When evaluated `<NAME>` will always result in `x`.

`: <NAME> ... ;`

`var <NAME> (x -> )` - defines new variable with initial value `x`.

`literal (x -> )` - takes the top of the stack and compiles its value as literal into current word definition.

`! (x v' -> )` stores `x` at the address of `v`. Works for `var`/`:` definitions interchangeably.

`@ (v' -> x)` loads value of `v`. Works for `const`/`var`/`:` definitions interchangeably.

## Development Aids

Words that starts from '\' are indended to be used during development
(or debugging). You should rarely need to use them outside of repl.

|Word|Signature|Description|
|---|---|---|
|`\c`|`( ... -> )`|clears the stack|
|`\i`|`( -> )`|prints info about current `niko` status|
|`\mem`|`( -> )`|prints detailed memory usage information|
|`\s`|`( -> )`|prints every stack item|

## Stack Manipulation

|Word|Signature|Description|
|---|---|---|
|`drop`|`(x -> )`|removes top item from the stack|
|`nip`|`(x y -> y )`|drops the second item on the stack|
|`dup`|`(x -> x x )`| duplicates top entry of the stack|
| `over`|`(x y -> x y x )`|places a copy of the second item on top of the stack|
|`rot`|`(x y z -> z x y )`|rotates top three items on the stack|
|`swap`|`(x y -> y x )`|exchanges top to items of the stack|
|`tuck`|`(x y -> y x y )`|places a copy of the top of the stack below the second item|
|`pick`|`( n -> x )`| places a copy of `n`-th element on top of the stack. `0 pick` equals `dup`|

## Creating Arrays

|Word|Signature|Description|
|---|---|---|
|`zeros`|`( s -> x )`|creates array with shape `s` filled with zeros|
|`ones`|`( s -> x )`|creates array with shape `s` filled with ones|
|`index`|`( s -> x )`|creates array with shape `s` filled values from `0` to the length-1|
|`pascal`|`( n -> x )`|creates `n*n` pascal matrix|
|`concat`|`(... s -> x)`|creates array of shape `s` out of preceding stack items|

## Array Introspection

|Word|Signature|Description|
|---|---|---|
|`len`|`(x -> n )`|replaces top array with total number of its elements
|`shape`|`(x -> x )`|replaces top array with 1-d array of its shape

## Manipulating Arrays

|Word|Signature|Description|
|---|---|---|
|`reshape`|`(x s -> y)`|reshapes `x` according to the shape `s` repeating or truncating if necessary
|`reverse`|`(x -> y)`|reverses all values in the array `x` keeping its shape

## Unary Operations

|Word|Signature|Description|
|---|---|---|
|`abs`|`(x -> y )`|replaces array with new array of the same type and shape with absolute values
|`neg`|`(x -> y )`|replaces array with new array of the same type and shape with negative values


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
|`div`|`(x y -> z)`|division quotient|
|`mod`|`(x y -> z)`|divisin remainder|
|`&`, `min`|`(x y -> z)`|min|
|`|`,`max` |`(x y -> z)`|max|
|`=`|`(x y -> b)`| equal comparison|
|`<`|`(x y -> b)`| less comparison|

## Aggregation

|Word|Signature|Description|
|---|---|---|
|`sum`|`(x -> y)`|sums all elements in `x` keeping only the result
|`sums`|`(x -> y)`|sums all elements in `x` organizing intermediate results in shape of `x`
|`deltas`|`(x -> y)`|computes deltas between consequent elements of `x`, leaving first element unchanged

## Higher Level Words

|Word|Signature|Description|
|---|---|---|
|`fold`|`(x op' -> y )`| fold cells of rank `0` using `op`
|`fold[]`|`(x r op' -> y )`| fold cells of rank `r` using `op`
|`scan`|`(x op' -> y )`| fold cells of rank `0` using `op` organizing all intermediate results in `x`'s shape
|`scan[]`|`(x r op' -> y )`| fold cells of rank `r` using `op` organizing all intermediate results in `x`'s shape
|`apply[]`|`(x r op' -> y )`| applies `op` to cells of rank `r` organizing results into `x`'s shape
|`power`|`(x n op` -> y)`|applies `op` `n` times to `x`
|`trace`|`(x s op` -> y)`|applies `op` multiple times to `x` and organizes intermediate results into `s` shape
|`pairwise`|`(x op' -> y )`| applies `op` to consequent pais of `x` leaving first element unchanged
|`pairwise[]`|`(x r op' -> y )`| applies `op` to consequent pais of cells of rank `r` leaving first element unchanged

## Input/Output

### `.`

`x ->`

## BLAS/LAPACK

### Low-level words

|Word|Signature|Description|
|---|---|---|
|`blas_gemm`|`(x y -> x * y )`|matrix multiplication
|`lapack_getrf`|`(x -> x ipiv info )`|matrix factorization
|`lapack_getri`|`(x ipiv -> x )`|matrix inversion

# Constants
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