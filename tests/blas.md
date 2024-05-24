# blas & lapack

## blas_gemm
```nkt
> [ 2 4 ] index 1. + [ 4 3 ] index 1. + blas_gemm .
[ [ 70. 80. 90. ] [ 158. 184. 210. ] ]
```
# lapack
## lapack_getrf
```nkt
> [ 2 2 ] index 0. + lapack_getrf . . .
0
[ 2 2 ]
[ [ 2. 3. ] [ 0. 1. ] ]
```
## lapack_getri
## 2x2
```nkt
> : m_inverse lapack_getrf drop lapack_getri ;
> [ 2 2 ] index 0. + m_inverse .
[ [ -1.5 0.5 ] [ 1. 0. ] ]
> [ 2 2 ] index 0. + dup m_inverse blas_gemm .
[ [ 1. 0. ] [ 0. 1. ] ]
```
## 3x3
```nkt
> 3 pascal 0. + m_inverse .
[ [ 3. -3. 1. ] [ -3. 5. -2. ] [ 1. -2. 1. ] ]
> 3 pascal 0. + dup m_inverse blas_gemm . 
[ [ 1. 0. 0. ] [ 0. 1. 0. ] [ 0. 0. 1. ] ]
```
## 4x4
```nkt
> 4 pascal 0. + m_inverse .
[ [ 4. -6. 4. -1. ] [ -6. 14. -11. 3. ] [ 4. -11. 10. -3. ] [ -1. 3. -3. 1. ] ]
> 4 pascal 0. + dup m_inverse blas_gemm . 
[ [ 1. -8.88178419700125e-16 0. 0. ] [ 8.88178419700125e-16 1. 0. -8.88178419700125e-16 ] [ 3.5527136788005e-15 5.32907051820075e-15 0.999999999999998 -1.77635683940025e-15 ] [ 3.5527136788005e-15 1.06581410364015e-14 -1.06581410364015e-14 1. ] ]
```
