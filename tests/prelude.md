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
# operations on arrays
## sum
```nkt
> 10 index sum .
45
> 10 index sums .
[ 0 1 3 6 10 15 21 28 36 45 ]
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
