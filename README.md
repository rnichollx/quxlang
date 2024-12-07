# QuxLang

QuxLang is a systems programming language. It's intended as an improvement over C++ for certain uses.

It is still in active development and is not "usable" yet.

I am compiling with something like:
```
-D LLVM_ROOT=/Users/rnicholl/Dev/llvm-root -D CMAKE_CXX_STANDARD=20 -D Boost_ROOT=/Users/rnicholl/Dev/boost_1_81_0 -D CMAKE_OSX_SYSROOT=/Users/rnicholl/Dev/MacOSX13.1.sdk -DGTest_DIR=/Users/rnicholl/Dev/googletest/gtest-export/lib/cmake/GTest -DCMAKE_CXX_FLAGS="-Wfatal-errors"
```

Currently, the compiler is not complete enough to "work" and all development is done with unit tests.

## Dependencies: 

* YAMLCpp
* GTest
* LLVM v16.0.6


## Status

Currently, some layers are mostly working. If you run the quxlang_gtests func_gen test, the compiler should 
compile `quxlang/tests/testdata/example`'s main function into the following IR. There is currently no translator
for QXVMIRv2 to LLVM IR, so machine code generation is not currently possible. I am currently working on the 
constexpr virtual machine, after the is done, machine code generation should be straightforward, and similar
to how v1 of the IR worked (which had working object code generation, albeit with no linker).

```
[Slots]:
    1: ARG NEW& I32
    2: LOCAL I32 // x
    3: LOCAL MUT& I32
    4: LITERAL NUMERIC_LITERAL 1
    5: LOCAL I32
    6: LOCAL WRITE& I32
    7: LOCAL I32 // y
    8: LOCAL MUT& I32
    9: LITERAL NUMERIC_LITERAL 3
    10: LOCAL I32
    11: LOCAL WRITE& I32
    12: BINDING MODULE(main)::foo BINDS %0
    13: LITERAL NUMERIC_LITERAL 3
    14: LITERAL NUMERIC_LITERAL 8
    15: LOCAL I32
    16: LOCAL I32
    17: LOCAL I32
    18: LOCAL MODULE(main)::buz // z
    19: LOCAL MUT& MODULE(main)::buz
    20: BINDING MODULE(main)::buz::.foo BINDS %19
    21: LOCAL -> I32 // a
    22: LOCAL MUT& -> I32
    23: LOCAL MUT& MODULE(main)::buz
    24: LOCAL MUT& I32
    25: LOCAL -> I32
    26: LOCAL WRITE& -> I32
    27: LOCAL MUT& -> I32
    28: LOCAL -> I32
    29: LOCAL CONST& -> I32
    30: LOCAL MUT& I32
    31: LOCAL MUT& -> I32
    32: LOCAL -> I32
    33: LOCAL CONST& -> I32
    34: LOCAL MUT& I32
    35: LITERAL NUMERIC_LITERAL 5
    36: LOCAL I32
    37: LOCAL I32
    38: LOCAL CONST& I32
    39: LOCAL I32
    40: LOCAL WRITE& I32
    41: LITERAL NUMERIC_LITERAL 4
[Blocks]:
BLOCK0 [<  >]:
    JUMP !1

BLOCK1 [<  >]:
    JUMP !3

BLOCK2 [<  >]:
    MISSING_TERMINATOR

BLOCK3 [<  >]:
    LCZ %2
    JUMP !4

BLOCK4 [< %2 >]:
    JUMP !5

BLOCK5 [< %2 >]:
    MKR %2, %3 // type1=I32 type2=MUT& I32
    LCI %5, 1
    ACF %3, %6, 0 // type1=MUT& I32 type2=WRITE& I32
    STR %5, %6
    JUMP !6

BLOCK6 [< %2 >]:
    JUMP !7

BLOCK7 [< %2 >]:
    LCZ %7
    JUMP !8

BLOCK8 [< %2, %7 >]:
    JUMP !9

BLOCK9 [< %2, %7 >]:
    MKR %7, %8 // type1=I32 type2=MUT& I32
    LCI %10, 3
    ACF %8, %11, 0 // type1=MUT& I32 type2=WRITE& I32
    STR %10, %11
    JUMP !10

BLOCK10 [< %2, %7 >]:
    JUMP !11

BLOCK11 [< %2, %7 >]:
    LCI %15, 3
    LCI %16, 8
    IVK MODULE(main)::foo #{I32, I32}, [RETURN=17, 15, 16]
    JUMP !12

BLOCK12 [< %2, %7 >]:
    JUMP !13

BLOCK13 [< %2, %7 >]:
    IVK MODULE(main)::buz::.CONSTRUCTOR #{@THIS NEW& MODULE(main)::buz}, [THIS=18]
    JUMP !14

BLOCK14 [< %2, %7, %18 >]:
    JUMP !15

BLOCK15 [< %2, %7, %18 >]:
    MKR %18, %19 // type1=MODULE(main)::buz type2=MUT& MODULE(main)::buz
    IVK MODULE(main)::buz::.foo #{@THIS AUTO& MODULE(main)::buz: MUT& MODULE(main)::buz}, [THIS=19]
    JUMP !16

BLOCK16 [< %2, %7, %18 >]:
    JUMP !17

BLOCK17 [< %2, %7, %18 >]:
    LCZ %21
    JUMP !18

BLOCK18 [< %2, %7, %18, %21 >]:
    JUMP !19

BLOCK19 [< %2, %7, %18, %21 >]:
    MKR %21, %22 // type1=-> I32 type2=MUT& -> I32
    MKR %18, %23 // type1=MODULE(main)::buz type2=MUT& MODULE(main)::buz
    ACF %23, %24, 0 // type1=MUT& MODULE(main)::buz type2=MUT& I32
    MPT %24, %25
    ACF %22, %26, 0 // type1=MUT& -> I32 type2=WRITE& -> I32
    STR %25, %26
    JUMP !20

BLOCK20 [< %2, %7, %18, %21 >]:
    JUMP !21

BLOCK21 [< %2, %7, %18, %21 >]:
    MKR %21, %27 // type1=-> I32 type2=MUT& -> I32
    ACF %27, %29, 0 // type1=MUT& -> I32 type2=CONST& -> I32
    LFR %29, %28
    DRP %28, %30
    MKR %21, %31 // type1=-> I32 type2=MUT& -> I32
    ACF %31, %33, 0 // type1=MUT& -> I32 type2=CONST& -> I32
    LFR %33, %32
    DRP %32, %34
    LCI %36, 5
    ACF %34, %38, 0 // type1=MUT& I32 type2=CONST& I32
    LFR %38, %37
    IADD %37, %36, %39
    ACF %30, %40, 0 // type1=MUT& I32 type2=WRITE& I32
    STR %39, %40
    JUMP !22

BLOCK22 [< %2, %7, %18, %21 >]:
    LCI %1, 4
    RET
```
