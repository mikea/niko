# Niko Reference

## Development Aids

Words that starts from '\' are indended to be used during development
(or debugging). You should rarely need to use them outside of repl.

|Word|Signature|Description|
|---|---|---|
|`\c`|`( ... -> )`|clears the stack
|`\i`|`( -> )`|prints info about current `niko` status
|`\mem`|`( -> )`|prints detailed memory usage information
|`\s`|`( -> )`|prints every stack item


## Stack Manipulation

|Word|Signature|Description|
|---|---|---|
|`drop`|`(x -> )`|removes top item from the stack
|`nip`|`(x y -> y )`|drops the second item on the stack
|`dup`|`(x -> x x )`| duplicates top entry of the stack
| `over`|`(x y -> x y x )`|places a copy of the second item on top of the stack
|`rot`|`(x y z -> z x y )`|rotates top three items on the stack
|`swap`|`(x y -> y x )`|exchanges top to items of the stack
|`tuck`|`(x y -> y x y )`|places a copy of the top of the stack below the second item
|`pick`|`( n -> x )`| places a copy of `n`-th element on top of the stack. `0 pick` equals `dup`.

## Creating Arrays

|Word|Signature|Description|
|---|---|---|
|`zeros`|`( s -> x )`|creates array with shape `s` filled with zeros
|`ones`|`( s -> x )`|creates array with shape `s` filled with ones
|`index`|`( s -> x )`|creates array with shape `s` filled values from `0` to the length-1
|`pascal`|`( n -> x )`|creates `n*n` pascal matrix


## Array Introspection

|Word|Signature|Description|
|---|---|---|
|`len`|`(x -> n )`|replaces top array with total number of its elements
|`shape`|`(x -> x )`|replaces top array with 1-d array of its shape

## Manipulating Arrays

|`reshape`|`(x s -> y)`|reshapes `x` according to the shape `s` repeating or truncating if necessary
|`reverse`|`(x -> y)`|reverses all values in the array `x` keeping its shape

## Unary Operations

|Word|Signature|Description|
|---|---|---|
|`abs`|`(x -> y )`|replaces array with new array of the same type and shape with absolute values
|`neg`|`(x -> y )`|replaces array with new array of the same type and shape with negative values


### Mathematical Functions

`x -> y`

## Binary Operations

|Word|Signature|Description|
|---|---|---|
|`+`|`(x y -> z)`| addition
|`-`|`(x y -> z)`| subtraction
|`*`|`(x y -> z)`| multiplication
|`/`|`(x y -> z)`| division
|`&`, `min`|`(x y -> z)`| min
|`|`, `max` |`(x y -> z)`| max
|`=`|`(x y -> b)`| equal comparison

## Aggregation

|`sum`|`(x -> y)`|sums all elements in `x` keeping only the result
|`sums`|`(x -> y)`|sums all elements in `x` organizing intermediate results in shape of `x`
|`deltas`|`(x -> y)`|computes deltas between consequent elements of `x`, leaving first element unchanged


## Higher Level Words

|Word|Signature|Description|
|---|---|---|
|`fold`|`(x op' -> y )`| fold cells of rank `0` using `op`
|`fold_rank`|`(x r op' -> y )`| fold cells of rank `r` using `op`
|`scan`|`(x op' -> y )`| fold cells of rank `0` using `op` organizing all intermediate results in `x`'s shape
|`scan_rank`|`(x r op' -> y )`| fold cells of rank `r` using `op` organizing all intermediate results in `x`'s shape
|`apply_rank`|`(x r op' -> y )`| applies `op` to cells of rank `r` organizing results into `x`'s shape
|`power`|`(x n op` -> y)`|applies `op` `n` times to `x`
|`trace`|`(x s op` -> y)`|applies `op` multiple times to `x` and organizes intermediate results into `s` shape
|`pairwise`|`(x op' -> y )`| applies `op` to consequent pais of `x` leaving first element unchanged
|`pairwise_rank`|`(x r op' -> y )`| applies `op` to consequent pais of cells of rank `r` leaving first element unchanged

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
