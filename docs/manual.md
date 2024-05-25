# niko

## Running

Niko can be started in variety of mode.
If you run `niko -h` then you will see help like this:

```
niko 3fe660e-dirty (1980-01-01T00:00:00Z) http://github.com/mikea/niko

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

### Expression Evaluation

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


## Syntax

Niko program consists of a sequence of words separated by whitespaces. 

Whitepsaces consist of space, tab and new line character.

Words are evaluated left to right.

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

Characters are 8-bit. 
Arrays of characters are called strings and are entered using quotes: `"abc"`.

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

