# Niko Examples

## Advanced Calculator

```nk
> 1 2 + .
3
```

## Temperature Convertion

```nk
> : centigrade 32 - 5 * 9. / ;
> : fahrenheit 9 * 5. / 32 + ;
> 72 centigrade .

> 21 fahrenheit .

> 11 index 10 * fahrenheit .


```


## Arrays

```nk
>  [ 1 2 3 ] dup + .
[ 2 4 6 ]
> [ 1 100 ] [ 2 3 ] reshape .
[ [ 1 100 1 ] [ 100 1 100 ] ]
```
