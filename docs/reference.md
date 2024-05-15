# Niko Reference

## Development Aids

|Word|Signature|Description|
|---|---|---|
|`\c`|`( ... -> )`|clears the stack

## Stack Manipulation

|Word|Signature|Description|
|---|---|---|
|`drop`|`( x -> )`|removes top item from the stack
|`nip`|`( x y -> y )`|drops the second item on the stack
|`dup`|`( x -> x x )`| duplicates top entry of the stack
| `over`|`( x y -> x y x )`|places a copy of the second item on top of the stack
|`rot`|`( x y z -> z x y )`|rotates top three items on the stack
|`swap`|`( x y -> y x )`|exchanges top to items of the stack
|`tuck`|`( x y -> y x y )`|places a copy of the top of the stack below the second item

## Creating Arrays

|Word|Signature|Description|
|---|---|---|
|`zeros`|`( s -> x )`|creates array with shape `s` filled with zeros
|`ones`|`( s -> x )`|creates array with shape `s` filled with ones
|`index`|`( s -> x )`|creates array with shape `s` filled values from `0` to the length-1
|`reshape`|`( x s -> y)`|reshapes `x` according to the shape `s` repeating or truncating if necessary


## Array Introspection

|Word|Signature|Description|
|---|---|---|
|`len`|`( x -> n )`|replaces top array with total number of its elements
|`shape`|`( x -> x )`|replaces top array with 1-d array of its shape

## Unary Operations

|Word|Signature|Description|
|---|---|---|
|`abs`|`( x -> y )`|replaces array with new array of the same type and shape with absolute values
|`neg`|`( x -> y )`|replaces array with new array of the same type and shape with negative values


### Mathematical Functions

`x -> y`

## Binary Operations

`x y -> z`

### `+`, `-`, `*`, `/`

## Higher Level Operations

|Word|Signature|Description|
|---|---|---|
|`fold`|`( x op' -> y )`| fold cells of rank `0` using `op`
|`fold_rank`|`( x r op' -> y )`| fold cells of rank `r` using `op`

## Input/Output

### `.`

`x ->`
