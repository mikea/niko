# niko Manual

## Running niko

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

````md
# Example Test

This block will be executed verbatim:

```nk
: add_2 2 + ;
```

This block has input/response structure
and looks like repl:

```nkt
> 1 add_2 .
3

````

See [example_test.md](example_test.md) for more
deliberate test example.