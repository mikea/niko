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

## Newton Method

Let's apply newton method to calculating `sqrt(2)`.
We need to solve `x^2-2=0`, 
single iteration: `x1=x-f(x)/f'(x)` in this case is `x=x-(x^2-2)/2x`:

dup dup * 2 - swap (x^2 x)
dup 2 * rot swap / - (x x^2/2x  )

```nk
: sqrt2_step dup dup * 2 - swap dup 2 * rot swap / - ;
```

```nkt
> 1. sqrt2_step .
1.5
> 1.5 sqrt2_step .
1.41666666666667
> 1. 3 sqrt2_step' power .
1.41421568627451
> 1. 100 sqrt2_step' power .
1.41421356237309
```

# Common Patterns

## Conditional execution

```nkt
> 10 0 cos' power .
10
> 10 1 cos' power .
-0.839071529076452
```