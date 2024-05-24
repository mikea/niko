# Interpreter tests

## atoms: entering and printing
### integers
```nkt
> 1 .
1
> -1 .
-1
> 9223372036854775807 .
9223372036854775807
> 9223372036854775808 .
9223372036854775807
> 0 .
0
```
### floats
```nkt
> 1. .
1.
> 1.0 .
1.
> 9007199254740992. .
9.00719925474099e+15
> 3.1415926 .
3.1415926
> -789.012 .
-789.012
> 1e3
> 1.23e3 .
1230.
> -4.56e-2 .
-0.0456
> 1e30 .
1e+30
> 1e-30 .
1e-30
> 0. .
0.
> 9. .
9.
```
### strings
```nkt
> "" .
""
> "abc" .
"abc"
> "this is test" .
"this is test"
> "abc" shape . "this is test" shape .
[ 3 ]
[ 12 ]
```
## arrays
```nkt
> [ ] .
[ ]
> [ 1 2 3 ] .
[ 1 2 3 ]
> [ [ 1 2 3 ] [ 4 5 6 ] ] .
[ [ 1 2 3 ] [ 4 5 6 ] ]
> [ [ 1 2 3 ] [ 4 5 6 ] ] shape .
[ 2 3 ]
> [ 1. 2. ] .
[ 1. 2. ]
> [ [ 1. 2. 3. ] [ 4. 5. 6. ] ] shape .
[ 2 3 ]
> [ "abc" "def" ] dup . shape .
[ "abc" "def" ]
[ 2 3 ]
> [ "" "1" "12" "123" ] dup . shape .
[ "" "1" "12" "123" ]
[ 4 ]
> [ [ 1. 2. 3. ] [ 4 5 6 ] ] dup . shape .
[ [ 1. 2. 3. ] [ 4 5 6 ] ]
[ 2 ]
```

# Word Definition

## `:`

```nkt
> : test_1 1 ; test_1 .
1
> 10 index test_1 + .
[ 1 2 3 4 5 6 7 8 9 10 ]
> : test_1+ test_1 + ; 10 index test_1+ .
[ 1 2 3 4 5 6 7 8 9 10 ]
> 10 index test_1+ .
[ 1 2 3 4 5 6 7 8 9 10 ]
> : 100+ 100 + ; 10 index 100+ .
[ 100 101 102 103 104 105 106 107 108 109 ]
> : centigrade 32 - 5 * 9. / ; 72 centigrade .
22.2222222222222
> : test_sum 0 +' fold_rank ;
> 10 index test_sum .
45
> : test_sums 0 +' scan_rank ; 
> 10 index test_sums .
[ 0 1 3 6 10 15 21 28 36 45 ]
```

## `const`

```nkt
> 42 const forty_two \s
> forty_two .
42
```

# error handling
```nkt
> 1999912+
ERROR: unknown word '1999912+'
```