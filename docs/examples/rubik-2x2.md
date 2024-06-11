# Exploring Rubik's 2x2 Cube

## Permutations

Define initial position:

```nk
8 index const I
```

And primitive moves permutations:

```nk
[ 3 0 1 2 4 5 6 7 ] const U
[ 1 2 3 0 4 5 6 7 ] const U`
[ 0 1 3 7 4 5 2 6 ] const F
[ 0 1 6 2 4 5 7 3 ] const F`
[ 0 2 6 3 4 1 5 7 ] const R
[ 0 5 1 3 4 6 2 7 ] const R`
[ 4 1 2 0 7 5 6 3 ] const L
[ 3 1 2 7 0 5 6 4 ] const L`
[ 0 1 2 3 5 6 7 4 ] const D
[ 0 1 2 3 7 4 5 6 ] const D`
```

Individual permutations can be applied using the `[]` word:

```nkt
> I U [] .
[ 3 0 1 2 4 5 6 7 ]
```

You can compare result with the original to detect all the cubes, 
that stayed in place.

```nkt
> I dup U [] = .
[ 0 0 0 0 1 1 1 1 ]
```

Let's quickly test that our permutation definitions look reasonable:

```nkt
> I dup U [] = .
[ 0 0 0 0 1 1 1 1 ]
> I dup U` [] = .
[ 0 0 0 0 1 1 1 1 ]
> I dup F [] = .
[ 1 1 0 0 1 1 0 0 ]
> I dup F` [] = .
[ 1 1 0 0 1 1 0 0 ]
> I dup R [] = .
[ 1 0 0 1 1 0 0 1 ]
> I dup R` [] = .
[ 1 0 0 1 1 0 0 1 ]
> I dup L [] = .
[ 0 1 1 0 0 1 1 0 ]
> I dup L` [] = .
[ 0 1 1 0 0 1 1 0 ]
> I dup D [] = .
[ 1 1 1 1 0 0 0 0 ]
> I dup D` [] = .
[ 1 1 1 1 0 0 0 0 ]
```

Permutations can be composed using the same `[]` word:

```nkt
> U R [] .
[ 3 1 6 2 4 0 5 7 ]
> U R [] I = .
[ 0 1 0 0 1 0 0 1 ]
```

Let's finish testing the primitives:

```nkt
> U U` [] I = .
[ 1 1 1 1 1 1 1 1 ]
> F F` [] I = .
[ 1 1 1 1 1 1 1 1 ]
> R R` [] I = .
[ 1 1 1 1 1 1 1 1 ]
> L L` [] I = .
[ 1 1 1 1 1 1 1 1 ]
> D D` [] I = .
[ 1 1 1 1 1 1 1 1 ]
```

## Exploring states

It is possible to apply multiple permutations to the same initial position 
to explore variations of the moves:

```nkt
> I [ U D ] [] .
[ [ 3 0 1 2 4 5 6 7 ] [ 0 1 2 3 5 6 7 4 ] ]
```

List of states can be compared to the initial the same way:

```
> I dup [ U D ] [] = .
```
