# Exploring Rubik's 2x2 Cube

## Primitive Moves

```nkt
> [ 3 0 1 2 4 5 6 7 ] const U
> 8 index dup U [] = .
[ 0 0 0 0 1 1 1 1 ]
> [ 1 2 3 0 4 5 6 7 ] const U`
> 8 index dup U` [] = .
[ 0 0 0 0 1 1 1 1 ]
> 8 index dup U [] U` [] = .
[ 1 1 1 1 1 1 1 1 ]
```

```nkt
> [ 0 1 3 7 4 5 2 6 ] const F
> 8 index dup F [] = .
[ 1 1 0 0 1 1 0 0 ]
> [ 0 1 6 2 4 5 7 3 ] const F`
> 8 index dup F` [] = .
[ 1 1 0 0 1 1 0 0 ]
> 8 index dup F [] F` [] = .
[ 1 1 1 1 1 1 1 1 ]
```

```nkt
> [ 0 2 6 3 4 1 5 7 ] const R
> 8 index dup R [] = .
[ 1 0 0 1 1 0 0 1 ]
> [ 0 5 1 3 4 6 2 7 ] const R`
> 8 index dup R` [] = .
[ 1 0 0 1 1 0 0 1 ]
> 8 index dup R [] R` [] = .
[ 1 1 1 1 1 1 1 1 ]
```

```nkt
> [ 4 1 2 0 7 5 6 3 ] const L
> 8 index dup L [] = .
[ 0 1 1 0 0 1 1 0 ]
> [ 3 1 2 7 0 5 6 4 ] const L`
> 8 index dup L` [] = .
[ 0 1 1 0 0 1 1 0 ]
> 8 index dup L [] L` [] = .
[ 1 1 1 1 1 1 1 1 ]
```

```nkt
> [ 0 1 2 3 5 6 7 4 ] const D
> 8 index dup D [] = .
[ 1 1 1 1 0 0 0 0 ]
> [ 0 1 2 3 7 4 5 6 ] const D`
> 8 index dup D` [] = .
[ 1 1 1 1 0 0 0 0 ]
> 8 index dup D [] D` [] = .
[ 1 1 1 1 1 1 1 1 ]
```

## Exploring state

```nkt
> [ ]
```