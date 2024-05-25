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
