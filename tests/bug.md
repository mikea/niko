```nk
: head 0 [] ;
: next_prime dup dup 0 [] mod not not repeat ;
```


```nkt
> 100 index 2 + 10 next_prime' trace 1 head' apply[]
```

```nk
: prime_upper_bound dup log log over log + * ceil ;
```

```nk
: prime dup prime_upper_bound index 2 + swap 1 - next_prime' power 0 [] ;
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
```
