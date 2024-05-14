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

### `len`

`x -> y`

### `shape`

`x -> y`

## Unary Operations

### `abs`, `neg`

`x -> y`

### Mathematical Functions

`x -> y`

## Binary Operations

`x y -> z`

### `+`, `-`, `*`, `/`

## Higher Level Operations

### fold

`x op' -> y`

### fold_by

`x axis op' -> y`

## Input/Output

### `.`

`x ->`
