Generate a 4096×4096 input with random 16-bit pixels:

```sh
{
  printf 'QIMG16LE\000\020\000\000\000\020\000\000'
  head -c 33554432 /dev/urandom
} > input.qimg
```

Confirm its size:

```sh
stat -c '%n %s bytes' input.qimg
```

Expected:

```text
input.qimg 33554448 bytes
```

Generate it once and use the same file for both benchmarks.