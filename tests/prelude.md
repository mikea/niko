# creating arrays

## zeros

```nkt
> 10 zeros .
[ 0 0 0 0 0 0 0 0 0 0 ]
> [ 10 ] zeros .
[ 0 0 0 0 0 0 0 0 0 0 ]
```

## ones

```nkt
> 10 ones .
[ 1 1 1 1 1 1 1 1 1 1 ]
> [ 10 ] ones .
[ 1 1 1 1 1 1 1 1 1 1 ]
```

# higher order words
## ,fold

```nkt
> 10 index +' ,fold .
45
> 10000 ones +' ,fold .
10000
> 10 index 1 + *' ,fold .
3628800
```

## ,scan

```nkt
> 10 index +' ,scan .
[ 0 1 3 6 10 15 21 28 36 45 ]
> 1000 ones +' ,scan +' ,fold .
500500
> 10 index 0. + +' ,scan .
[ 0. 1. 3. 6. 10. 15. 21. 28. 36. 45. ]
```

## ,pairwise

```nkt
> 10 index +' ,pairwise .
[ 0 1 3 5 7 9 11 13 15 17 ]
```

# shorthands

## sum

```nkt
> 6 index sum .
15
```

## sums

```nkt
> 6 index sums .
[ 0 1 3 6 10 15 ]
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

# mathematical functions

## gcd

```nkt
> 12 8 gcd .
4
> 15 10 gcd .
5
> 17 13 gcd .
1
> 48 18 gcd .
6
> 100 40 gcd .
20
```

## lcm

```nkt
> 12 8 lcm .
24
> 15 10 lcm .
30
> 4 6 lcm .
12
> 21 14 lcm .
42
> 9 12 lcm .
36
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

## Forth-style utilities

### Basic arithmetic
```nkt
> 5 1+ .
6
> 10 1- .
9
> 7 2* .
14
> 20 2/ .
10.
> [ 1 2 3 ] 1+' ,apply .
[ 2 3 4 ]
> [ 5 4 3 ] 1-' ,apply .
[ 4 3 2 ]
```

### Modulo and comparison
```nkt
> 5 2mod .
1
> 4 2mod .
0
> 0 0= .
1
> 5 0= .
0
> 0 0<> .
0
> 5 0<> .
1
> [ 0 1 0 2 0 ] 0=' ,apply .
[ 1 0 1 0 1 ]
> [ 2 3 4 5 6 ] 2mod' ,apply .
[ 0 1 0 1 0 ]
```