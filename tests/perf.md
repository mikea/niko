```nk
: head 0 [] ;
: next_prime dup dup 0 [] mod not not repeat ;
: prime_upper_bound dup log log over log + * ceil ;
: prime dup prime_upper_bound index 2 + swap 1 - next_prime' ,power head ;
10000 prime .
```
