# Example Test

Tests are normal markdown files and can be used
as documentation.


## `nk` blocks

`nk` blocks are executed as written. 
Typically this is where reusable definitions go:

```nk
: add_2 2 + ;
```

## `nkt` blocks

`nkt` blocks contain actual tests:

```nkt
> 1 add_2 .
3
```

If there's any output mismatch, test execution will fail.