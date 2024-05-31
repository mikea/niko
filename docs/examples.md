# Niko Examples

## Advanced Calculator

```nkt
> 1 2 + .
3
```

## Temperature Convertion

```nkt
> : celcius 32 - 5 * 9. / ;
> : fahrenheit 9 * 5. / 32 + ;
> 72 celcius .
22.2222222222222
> 21 fahrenheit .
69.8
> 11 index 10 * dup . fahrenheit .
[ 0 10 20 30 40 50 60 70 80 90 100 ]
[ 32. 50. 68. 86. 104. 122. 140. 158. 176. 194. 212. ]
> 10 index 10 * dup . celcius .
[ 0 10 20 30 40 50 60 70 80 90 ]
[ -17.7777777777778 -12.2222222222222 -6.66666666666667 -1.11111111111111 4.44444444444444 10. 15.5555555555556 21.1111111111111 26.6666666666667 32.2222222222222 ]
```

## Fibonacci numbers

```nk
: next_fib dup sum swap -1 [] swap 2 cat ;
: fib [ 0 1 ] swap next_fib' ,power 0 [] ;
```

```nkt
> [ 1 1 ] next_fib .
[ 1 2 ]
> [ 1 2 ] next_fib .
[ 2 3 ]
> [ 1 1 ] 10 next_fib' ,power .
[ 89 144 ]
> 10 fib .
55
> 40 fib .
102334155
```

## Newton Method

Let's apply newton method to calculating `sqrt(2)`.
We need to solve `x^2-2=0`, 
single iteration: `x1=x-f(x)/f'(x)` in this case is `x=x-(x^2-2)/2x`:

```nk
: sqrt2_step dup dup * 2 - swap dup 2 * rot swap / - ;
```

```nkt
> 1. sqrt2_step .
1.5
> 1.5 sqrt2_step .
1.41666666666667
> 1. 3 sqrt2_step' ,power .
1.41421568627451
> 1. 10 sqrt2_step' ,power .
1.41421356237309
```

## Prime Numbers

```nk
: head 0 [] ;
: next_prime dup dup 0 [] mod not not repeat ;
```

```nkt
> [ 2 3 4 5 6 7 ] next_prime .
[ 3 5 7 ]
```

$p_n < n(log n + log log n), n>=6$

```nk
: prime_upper_bound dup log log over log + * ceil ;
: prime dup prime_upper_bound index 2 + swap 1 - next_prime' ,power head ;
: primes dup prime_upper_bound index 2 + swap 1 - next_prime' ,trace head' ,apply ;
```

```nkt
> 20 prime_upper_bound .
82
> 100 prime_upper_bound .
614
> 1000 prime_upper_bound .
8841
> 20 prime .
71
> 100 prime .
541
> 20 primes .
[ 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 ]
```

## Common Patterns

### Conditional execution

```nkt
> 10 0 cos' ,power .
10
> 10 1 cos' ,power .
-0.839071529076452
```

## Project Euler

[https://projecteuler.net/](https://projecteuler.net/about)

### Problem 1

```nkt
> 1000 index dup 3 mod over 5 mod & not * sum .
233168
```

### Problem 2

```nk
: fibs [ 0 1 ] swap next_fib' ,trace head' ,apply ;
```

```nkt
>  10 fibs .
[ 1 1 2 3 5 8 13 21 34 55 ]
> 40 fibs dup 4000000 < 
> over 2 mod not & * sum .
4613732
```

### Problem 3

```nkt
> 600851475143 
> 1000 primes over over mod not repeat .
[ 71 839 1471 6857 ]
```

### Problem 7


```nkt
> 1001 prime .
7927
```

### Problem 10
