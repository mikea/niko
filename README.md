# Niko

Niko is stack-oriented array programming language. It is heavily influenced by Forth and Lisp.

## Syntax

Whitepsaces consist of space, tab and new line character. Words are sequences of non-whitespace symbols.

Words are normally evaluated left to right.

### Arrays

Array is the only value in the language. Everything is represented as an array.

Array has a rank >= 0, and rank number of dimensions. 

Array has a length and it is always equals to the product of its dimensions. 

Arrays of rank 0 are called sacalars and always have 1 element. 

Dimensions have to either be all non-zero or all zero. 

Following element types are supported:
- char
- i64
- f64


#### Characters

Strings are sequences if characters: `"abc"`.

#### Integers

Integers are 64-bit wide: `0`, `1024`, `-25`.

#### Floats

Floats are 64-bit wide. Float literal must contain `.` or `e` symbols:
`0.`, `1e-16`, `-3.1415`.

#### Array Literals

To create an array literal use `[` `]` words to enter them (remember the spaces):

```
> [ 1 2 3 ] .
> [ [ 1. 2. 3. ] [ 4. 5. 6. ] ]
```

