# QuxLang

QuxLang is a systems programming language. It's intended as an improvement over C++ for certain uses.

It is still in active development and is not "usable" yet.

I am compiling with something like:
```
-D LLVM_ROOT=/Users/rnicholl/Dev/llvm-root -D CMAKE_CXX_STANDARD=20 -D Boost_ROOT=/Users/rnicholl/Dev/boost_1_81_0 -D CMAKE_OSX_SYSROOT=/Users/rnicholl/Dev/MacOSX13.1.sdk -DGTest_DIR=/Users/rnicholl/Dev/googletest/gtest-export/lib/cmake/GTest -DCMAKE_CXX_FLAGS="-Wfatal-errors"
```

I am testing the executable with:

quxlang_main /Users/rnicholl/Dev/quxlang/example.sr

## Dependencies: 

* YAMLCpp
* GTest
* LLVM v16.0.6

