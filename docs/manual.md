# niko

## Running

Niko can be started in several modes: file execution, REPL, expression evaluation, or test running.
If you run `niko -h` then you will see help with quick overview:

```text
niko d082e4f-dirty Debug (2025-07-02T03:11:48Z) http://github.com/mikea/niko

USAGE:
    bin/niko [FLAGS] [OPTIONS] [FILE]

FLAGS:
    -z               Do not load the prelude
    -v               Verbose test execution
    -f               Fail fast
    -h               Print help information

OPTIONS:
    -e <expr>        Evaluate niko expression
    -t <test.md>     Run markdown test

ARGS:
    <FILE>           Execute niko source file

Enters the repl if no options or files are specified (using rlwrap is recommended).
```

### File Execution

The simplest way to run a Niko program is to pass the filename as an argument:

```bash
niko program.nk
```

This will execute the contents of `program.nk` and exit. For example:

```
( program.nk - Calculate sum of squares )
5 index     ( Generate [0 1 2 3 4] )
dup *       ( Square each element )
sum         ( Sum all squares )
.           ( Print result )
```

Running `niko program.nk` outputs: `30`

### REPL (Read-Eval-Print Loop)

When started without arguments, Niko enters interactive mode:

```bash
niko
```

In REPL mode:
- The stack contents are displayed before each prompt
- You can enter expressions interactively
- Use `\c` to clear the stack
- Use `\s` to show all stack items with indices
- Use `\i` for system information
- Press Ctrl+D or Ctrl+C to exit

Example REPL session:
```
niko d082e4f-dirty Debug (2025-07-02T03:11:48Z) http://github.com/mikea/niko
 > 2 3 +
5  > dup *
5 25  > .
25
 > 
```

Using `rlwrap` is recommended for command history and line editing:
```bash
rlwrap niko
```

### Expression Evaluation

Use `-e` to evaluate a single expression from the command line:

```bash
niko -e "2 3 + ."
```

This is useful for:
- Quick calculations
- Shell scripting
- Testing small code snippets

Examples:
```bash
# Simple arithmetic
niko -e "10 3 / ."

# String manipulation
niko -e '"Hello" " World" + .'

# Using multiple statements
niko -e "5 index dup * ."
```

### Pipe Input

Niko can read programs from standard input, making it composable with Unix tools:

```bash
echo "2 3 + ." | niko
cat program.nk | niko
niko < program.nk
```

This is particularly useful for:
- Generating Niko code dynamically
- Processing output from other programs
- Creating filter programs

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

### Command-Line Flags

#### `-z` - No Prelude
Starts Niko without loading the standard prelude. Useful for:
- Testing in a minimal environment
- Debugging prelude issues
- Creating custom prelude replacements

```bash
niko -z -e "1 2 + ."  # Works - basic operations are built-in
niko -z -e "PI ."     # Error - PI is defined in prelude
```

#### `-v` - Verbose Test Execution
Shows each test line as it executes. Helpful for debugging failing tests:

```bash
niko -v -t test.md
```

#### `-f` - Fail Fast
Stops test execution on the first failure instead of running all tests:

```bash
niko -f -t tests/*.md
```

## Syntax

Niko program consists of a sequence of words separated by whitespaces. 

Whitepsaces consist of space, tab and new line character.

Words are evaluated left to right.

Comments are encolsed by `()`: `( this is niko comment )`.

## Data Types

Niko is based on a unified array model where **everything is an array**. There are no scalar values in Niko - even single numbers and characters are represented as arrays of length 1.

### Core Array Types

The language supports six fundamental array element types:

| Type | Internal Name | Description | C++ Type |
|------|---------------|-------------|----------|
| `c8` | T_C8 | 8-bit characters | `char` |
| `i64` | T_I64 | 64-bit signed integers | `int64_t` |
| `f64` | T_F64 | 64-bit floating point | `double` |
| `arr` | T_ARR | Nested arrays | `array_p` |
| `ffi` | T_FFI | Foreign function interface | `function pointer` |
| `dict` | T_DICT_ENTRY | Dictionary entries | `uint64_t` |

The first four types (`c8`, `i64`, `f64`, `arr`) are user-visible and form the basis of all Niko programming. The last two (`ffi`, `dict`) are internal implementation types.

### Array Properties

Every array in Niko has two important properties:

#### 1. Atom Flag (`a`)
Arrays can be **atoms** or **vectors**:
- **Atom**: A single-element array that prints without brackets (e.g., `42`, `3.14`, `"a"`)
- **Vector**: A multi-element array that prints with brackets (e.g., `[ 1 2 3 ]`)

```nkt
> 42 .        ( atom - prints without brackets )
42
> [ 42 ] .    ( vector - prints with brackets )
[ 42 ]
```

#### 2. Quote Flag (`q`)
Used internally for compilation and metaprogramming - indicates whether an array represents quoted code.

### Type Details

#### Characters (`c8`)

8-bit characters form the basis of string handling. Single characters and strings are both character arrays:

```nkt
> "a" .
"a"
> "hello" .
"hello"
> "hello" len .
5
```

#### Integers (`i64`)

Signed 64-bit integers. Literals must not contain decimal points:

```nkt
> 42 .
42
> -1000000000000 .
-1000000000000
> 0xFF .           ( hexadecimal not supported in literals )
ERROR: unknown word '0xFF'
```

#### Floats (`f64`)

64-bit double-precision floating point numbers. Literals must contain `.` or `e`:

```nkt
> 3.14159 .
3.14159
> 1e-10 .
1e-10
> 42. .            ( note the dot - makes it a float )
42.
```

#### Nested Arrays (`arr`)

Arrays can contain other arrays, enabling multidimensional data structures:

```nkt
> [ [ 1 2 ] [ 3 4 ] ] .
[ [ 1 2 ] [ 3 4 ] ]
> [ "hello" [ 1 2 3 ] 42. ] .
[ "hello" [ 1 2 3 ] 42. ]
```

### Type Conversion

#### Implicit Conversions

Niko performs several implicit type conversions during operations:

##### Numeric Type Promotion
In binary operations between different numeric types, the result type is automatically promoted:

```nkt
> 5 3.14 + .       ( i64 + f64 → f64 )
8.14
> 2.5 4 * .        ( f64 * i64 → f64 )
10.
> 10 3 / .         ( i64 / i64 → f64 for division )
3.33333333333333
```

##### Character in Numeric Operations
Characters (`c8`) participate in numeric operations with special promotion rules:

```nkt
> "A" 0 + .        ( c8 + i64 → i64 )
[ 65 ]
> "abc" "ABC" - .  ( c8 - c8 → c8 )
"   "
> "ABC" 32 + .     ( c8 + i64 → i64 )
[ 97 98 99 ]
> "abc" 32 - .     ( c8 - i64 → i64 - uppercase )
[ 65 66 67 ]
```

When both operands are characters, the result remains a character. When mixed with integers or floats, the result is promoted to the numeric type.

##### Array Broadcasting
Single-element arrays (atoms) are automatically broadcast in operations with larger arrays:

```nkt
> [ 1 2 3 ] 10 + . ( broadcast scalar to array length )
[ 11 12 13 ]
> 5 [ 1 2 3 ] * .  ( scalar * array )
[ 5 10 15 ]
```

#### Explicit Conversions

Type conversion functions allow explicit control over data types:

##### Basic Type Conversions

```nkt
> 42 f64 .         ( integer to float )
42.
> 3.14 i64 .       ( float to integer - truncates )
3
> 65 c8 .          ( integer to character )
'A'
```

##### String/Number Parsing

The `i64` and `f64` functions can parse string representations:

```nkt
> "42" i64 .       ( parse string to integer )
42
> "3.14" f64 .     ( parse string to float )
3.14
> "invalid" i64 .  ( invalid format causes error )
ERROR: invalid number format: 'invalid'
```

##### Number to String Conversion

The `str` function converts numbers to their string representation:

```nkt
> 42 str .         ( number to string )
"42"
> 3.14 str .       ( float to string )
"3.140000"
> [ 1 2 3 ] str .  ( array of numbers to array of strings )
[ "1" "2" "3" ]
```

### Array Creation

Arrays can be created in several ways:

#### Literals
```nkt
> [ 1 2 3 ] .
[ 1 2 3 ]
```

#### Generation Functions
```nkt
> 5 zeros .        ( array of 5 zeros )
[ 0 0 0 0 0 ]
> 3 ones .         ( array of 3 ones )
[ 1 1 1 ]
> 4 index .        ( array [0,1,2,3] )
[ 0 1 2 3 ]
```

#### Stack Concatenation
```nkt
> 1 2 3  3 cat .   ( collect 3 items from stack )
[ 1 2 3 ]
```

### Memory Management

Arrays use reference counting for automatic memory management. SIMD-aligned allocation is used for large arrays to optimize vector operations. Arrays are immutable by default - operations create new arrays rather than modifying existing ones.

### Type System Philosophy

The unified array model eliminates the scalar/vector distinction found in many languages:
- Operations work uniformly on single elements and arrays
- No need for separate scalar and vector versions of functions  
- Enables powerful array programming patterns
- Simplifies the mental model - everything follows the same rules

## Higher-Order Words (Adverbs)

Niko provides powerful higher-order words (called "adverbs" in array programming) that take other words as arguments. These enable functional programming patterns and array processing without explicit loops.

### Word Quotation

To pass a word as an argument to a higher-order function, append a single quote (`'`) to the word name:

```nkt
> [ 1 2 3 4 ] +' ,fold .    ( sum using fold )
10
> [ 1 2 3 ] neg' ,apply .   ( negate each element )
[ -1 -2 -3 ]
```

### Core Higher-Order Words

#### `,fold` - Reduce an Array

Applies a binary operation between elements, accumulating a result:

```nkt
> [ 1 2 3 4 ] +' ,fold .    ( 1 + 2 + 3 + 4 )
10
> [ 5 3 8 2 ] max' ,fold .  ( maximum value )
8
> [ 10 20 30 40 ] +' ,fold .
100
```

**How it works**: Takes the first element as initial value, then applies the operation between the accumulator and each subsequent element.

#### `,scan` - Running Fold

Like `,fold` but keeps all intermediate results:

```nkt
> [ 1 2 3 4 ] +' ,scan .    ( running sum )
[ 1 3 6 10 ]
> [ 5 3 8 2 ] max' ,scan .  ( running maximum )
[ 5 5 8 8 ]
> [ 1 2 3 4 ] *' ,scan .    ( running product )
[ 1 2 6 24 ]
```

**How it works**: Same as `,fold` but collects each intermediate accumulator value into an array.

#### `,apply` - Map Over Array

Applies an operation to each element:

```nkt
> [ 1 2 3 ] neg' ,apply .   ( negate each )
[ -1 -2 -3 ]
> [ 1 4 9 ] sqrt' ,apply .  ( square root each )
[ 1. 2. 3. ]
> : double 2 * ;
> [ 1 2 3 ] double' ,apply .  ( double each element )  
[ 2 4 6 ]
```

**How it works**: Pushes each element onto the stack as an atom, executes the operation, then collects all results with `cat`. Note that operations that produce multiple values will have all values collected.

#### `,pairwise` - Apply Between Adjacent Pairs

Applies a binary operation between consecutive elements:

```nkt
> [ 1 3 7 10 ] swap-' ,pairwise .   ( differences )
[ 1 2 4 3 ]
> : swap/ swap / ;
> [ 1 2 4 8 ] swap/' ,pairwise .   ( ratios )
[ 1 2. 2. 2. ]
> [ 5 10 20 ] *' ,pairwise .
[ 5 50 200 ]
```

**How it works**: Keeps the first element unchanged, then applies the operation between each adjacent pair.

#### `,power` - Iterate a Function

Applies an operation n times to a value:

```nkt
> : double dup + ;
> 1 8 double' ,power .       ( double 8 times: 1→2→4→...→256 )
256
> : inc 1 + ;
> 0 5 inc' ,power .          ( increment 5 times: 0→1→2→3→4→5 )
5
> 5 4 1 +' ,power .         ( add 1 four times )
9
```

**How it works**: Starting with the initial value on stack, applies the operation n times.

#### `,collect` - Collect Results

Runs an operation n times and collects the top n stack items:

```nkt
> : next dup 1 + ;
> 1 5 next' ,collect .        ( generate 1,2,3,4,5 )
[ 1 2 3 4 5 ]
> 1 3 dup' ,collect .         ( duplicate 3 times )
[ 1 1 1 ]
```

**How it works**: Executes the operation n times, drops the top item, then concatenates the top n items from the stack.

#### `,trace` - Trace Execution

Like `,power` but keeps all intermediate results:

```nkt
> : double dup + ;
> 1 5 double' ,trace .       ( powers of 2 )
[ 2 4 8 16 32 ]
> : inc 1 + ;
> 1 4 inc' ,trace .          ( sequence 2,3,4,5 )
[ 2 3 4 5 ]
```

**How it works**: Duplicates the top value before each operation (except the first), collecting all states.

### Practical Examples

#### Statistics
```nkt
> [ 3 1 4 1 5 9 ] dup +' ,fold swap len / .  ( mean )
3.83333333333333
> [ 1 2 3 4 5 ] dup +' ,fold swap len / .    ( mean )
3.
```

#### Array Manipulation
```nkt
> [ [ 1 2 ] [ 3 4 ] [ 5 6 ] ] +' ,fold .
[ 9 12 ]
> [ 1 2 3 ] dup + .
[ 2 4 6 ]
```

#### Cumulative Operations
```nkt
> 10 index 1 + *' ,scan .    ( factorials )
[ 1 2 6 24 120 720 5040 40320 362880 3628800 ]
> [ 100 -20 30 -50 ] +' ,scan .  ( running balance )
[ 100 80 110 60 ]
```

### Understanding Quoted Words

The quote syntax (`'`) creates a reference to a word without executing it. This reference can then be passed to higher-order words:

```nkt
> +' .                      ( print the quoted word )
+'
> [ 1 2 ] [ 3 4 ] + .      ( normal addition )
[ 4 6 ]
> 1 2 + .                   ( normal scalar addition )
3
```

Higher-order words are fundamental to Niko's array programming paradigm, allowing complex operations to be expressed concisely without explicit loops or recursion.

### Word Fusing Optimization

Niko supports **word fusing** as a performance optimization for higher-order words. When executing an adverb (word starting with comma) with a quoted word on the stack, the interpreter first checks for a specialized fused implementation.

#### How It Works

When you execute `word' ,adverb`, Niko automatically looks for a word named `word,adverb`. If found, it executes the fused version instead of the generic adverb implementation.

```niko
: dup,fold dup * sum ;  ( Optimized: square and sum )
: neg,apply neg' ,apply ;  ( Fallback: use normal implementation )

[ 1 2 3 ] dup' ,fold    ( Executes dup,fold → squares then sums )
[ 1 2 3 ] neg' ,apply   ( Executes neg,apply → normal negation )
```

#### Benefits

- **Performance**: Specialized implementations can be much faster than generic higher-order words
- **Clarity**: Common patterns can have more descriptive implementations
- **Zero overhead**: No performance penalty when fused words aren't defined
- **Transparent**: Falls back automatically to normal execution

#### Common Patterns

```niko
( Mathematical optimizations )
: dup,fold dup * sum ;           ( sum of squares )
: sqrt,apply sqrt' ,apply ;      ( vectorized square root )

( Aggregation shortcuts )  
: +,fold sum ;                   ( use built-in sum )
: *,scan *' ,scan ;              ( running product )

( Custom combinations )
: double,apply 2 *' ,apply ;     ( double all elements )
: abs,fold abs' ,apply sum ;     ( sum of absolute values )
```

The fusing system enables building libraries of optimized operations while maintaining the composability of higher-order words.
