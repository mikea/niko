```nk
: head 0 [] ;
: next_prime dup dup 0 [] mod not not repeat ;
: prime_upper_bound dup log log over log + * ceil ;

: next_primes ( tail -> prime tail )
    dup 0 [] 2dup mod not not rot swap repeat ;

: primes_helper ( steps max_n -> primes )
    index 2 + swap next_primes' ,collect ;

: primes (n -> list_of_n_primes) 
    dup prime_upper_bound primes_helper ;

1000 primes
```
