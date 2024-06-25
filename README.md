# Niko

Niko is a stack-oriented concatenative vector programming language. 

## Building and Running

Only Linux is supported.

- `apt install just re2c valgrind ninja-build libjemalloc-dev`
- `just release`
- `bin/niko`

## Language Overview

### Forth Comparison

Niko is very similar to Forth: 
stack manipulation core eval loop, 
dictionary structure and many 
syntax elements come from it. 

Niko differences from Forth are:
- all values are vectors
- words are represented as vectors too (lisp like)
- Niko doesn't try to self-host
- low-level concepts such as return stack are not exposed

## Documentation

- [User Manual](docs/manual.md)
- [Code Examples](docs/examples.md)
- [Test Example](docs/example_test.md)
- [Reference](docs/reference.md)
