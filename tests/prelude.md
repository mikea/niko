# creating arrays

## zeros

```nkt
> 10 zeros .
[ 0 0 0 0 0 0 0 0 0 0 ]
> [ 10 ] zeros .
[ 0 0 0 0 0 0 0 0 0 0 ]
> [ 2 3 ] zeros .
[ [ 0 0 0 ] [ 0 0 0 ] ]
```

## ones

```nkt
> 10 ones .
[ 1 1 1 1 1 1 1 1 1 1 ]
> [ 10 ] ones .
[ 1 1 1 1 1 1 1 1 1 1 ]
> [ 2 3 ] ones .
[ [ 1 1 1 ] [ 1 1 1 ] ]
```

# higher order words
## fold
```nkt
> 10 index +' fold .
45
> 10000 ones +' fold .
10000
> 10 index 1 + *' fold .
3628800
> [ 2 3 4 ] index +' fold .
276
```
## scan
```nkt
> 10 index +' scan .
[ 0 1 3 6 10 15 21 28 36 45 ]
> 1000 ones +' scan +' fold .
500500
> 10 index 0. + +' scan .
[ 0. 1. 3. 6. 10. 15. 21. 28. 36. 45. ]
> [ 2 3 4 ] index +' scan .
[ [ [ 0 1 3 6 ] [ 10 15 21 28 ] [ 36 45 55 66 ] ] [ [ 78 91 105 120 ] [ 136 153 171 190 ] [ 210 231 253 276 ] ] ]
```
## pairwise
```nkt
> 10 index +' pairwise .
[ 0 1 3 5 7 9 11 13 15 17 ]
```
# shorthands

## sum
```nkt
> [ 2 3 ] index sum .
15
```

## sums
```nkt
> [ 2 3 ] index sums .
[ [ 0 1 3 ] [ 6 10 15 ] ]
```

## min

```nkt
> 3 index 1 min .
[ 0 1 1 ]
```

## max

```nkt
> 3 index 1 max .
[ 1 1 2 ]
```

## swap-

```nkt
> 3 4 - . 3 4 swap- .
-1
1
```

## deltas

```nkt
> [ 1 3 6 8 0 ] deltas .
[ 1 2 3 2 -8 ]
```

# constants

```nkt
> E . E log .
2.71828182845905
1.
> PI . PI cos .
3.14159265358979
-1.
> PI/2 2 * PI = .
1
> PI/4 4 * PI = .
1
> 1/PI PI * .
1.
> 2/PI PI * .
2.
```