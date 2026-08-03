Generate a 4096×4096 input with random 16-bit pixels:

The benchmark performs 256 separable blur iterations, keeping I/O small relative to the computation.

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

On macOS, use:

```sh
stat -f '%N %z bytes' input.qimg
```

Expected:

```text
input.qimg 33554448 bytes
```

Generate it once and use the same file for both benchmarks.

## macOS ARM64

Build the Quxlang target into the ignored project `tmp/` directory:

```sh
QXC_TARGETS=integer-blur-macos-arm64 \
QXC_OUTPUT_DIR="$PWD/tmp/integer-blur-macos/quxlang" \
TMPDIR="$PWD/tmp" \
local/qxc-compile-testmodule.sh
```

The Quxlang executable is:

```text
tmp/integer-blur-macos/quxlang/output/integer-blur-macos-arm64/app
```

Build the portable C++ version without native CPU-specific instructions:

```sh
mkdir -p tmp/integer-blur-macos/cpp
clang++ -O3 -DNDEBUG -std=c++23 \
  benchmarks/integer_blur/integer_blur.cpp \
  -o tmp/integer-blur-macos/cpp/integer_blur
```

Run each executable three times against the same input:

```sh
for run in 1 2 3; do
  /usr/bin/time -p tmp/integer-blur-macos/quxlang/output/integer-blur-macos-arm64/app \
    input.qimg "tmp/integer-blur-macos/quxlang-$run.qimg"
done

for run in 1 2 3; do
  /usr/bin/time -p tmp/integer-blur-macos/cpp/integer_blur \
    input.qimg "tmp/integer-blur-macos/cpp-$run.qimg"
done
```
