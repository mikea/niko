# niko

## Running

Niko can be started in several modes: repl, test or eval.
If you run `niko -h` then you will see help with quick overview:

```text
niko ecc731b Release (2024-05-25T18:35:35Z) http://github.com/mikea/niko

USAGE:
    bin/niko [FLAGS] [OPTIONS]

FLAGS:
    -z               Do not load the prelude
    -v               Verbose test execution
    -h               Print help information

OPTIONS:
    -e <expr>        Evaluate niko expression
    -t <test.md>     Run markdown test

Enters the repl if no options are specified (using rlwrap is recommended).
```

### REPL

### Evaluation

### Tests

Niko tests are ordinary markdown files. To execute the test run `niko -t <test_file.md>`.

Inside the file system looks for markdown code markers `` ```nk `` and `` ```nkt ``.

The code withing `nk` block is evaluated as is.
The output (if any) will be sent to stdout.

The code within `nkt` block has the input-and-response structure,
where interpreter input is on lines starting with `>` (like repl), and everything else is the output. The system checks the output against the expected one and fails if there's any mismatch.

#### Test Example

`test.md`:

````md
# Simple Test

This `nk` block will be executed verbatim:

```nk
: add_2 2 + ;
```

This `nkt` block has input/response structure
and looks like repl:

```nkt
> 1 add_2 .
3
```

````

See [example_test.md](example_test.md) for more
deliberate test example.
s

## Syntax

Niko program consists of a sequence of words separated by whitespaces. 

Whitepsaces consist of space, tab and new line character.

Words are evaluated left to right.

Comments are encolsed by `()`: `( this is niko comment )`.

### Arrays

Array is the only value in the language. Everything is represented as an array.

Following element types are supported:

- char
- i64
- f64

#### Characters

Characters are 8-bit.
Arrays of characters are called strings and are entered using quotes:

```nkt
> "abc" .
"abc"
```

#### Integers

Integers are signed, 64-bit wide and do not
contain dot symbol: 

```nkt
> 0 1024 -42 . . .
-42
1024
0
```

#### Floats

Floats are 64-bit wide. Float literal must contain `.` or `e` symbols:
`0.`, `1e-16`, `-3.1415`.

#### Array Literals

To create an array literal use `[` `]` words to enter them (remember the spaces):

```nkt
> [ 1 2 3 ] .
[ 1 2 3 ]
> [ [ 1. 2. 3. ] [ 4. 5. 6. ] ] .
[ [ 1. 2. 3. ] [ 4. 5. 6. ] ]
```
