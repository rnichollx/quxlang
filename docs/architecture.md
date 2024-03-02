# Qux Compiler (qxc) Architecture

Note: Some stages of this architecture are not yet fully implemented in the
compiler.

## Data Layers

There are several layers in the Qux Compiler (qxc) architecture. Each layer has
a specific data format that is used to
communicate with the next layer. The layers are as follows:

### Frontend Compilation Layers

1. **Source Code**: The source code is the input to the compiler. The Qux
   Compiler accepts Qux sources, as well as
   assembly, module definitions, and buildfiles. There are 2 sublayers:

    1. **External Source Code**: This consists of the code external to the qxc
       build process. For example, a set of
       files on disk, or on a network share. This is the input to the qxc build
       process.

    2. **Build Source**: The qxc compiler takes a snapshot of the external
       source code before it begins the compilation
       process. This means that modifications to the source code directory
       during the compilation process will not cause
       inconsistencies in the output. This differs from traditional C++ build
       systems, like CMake + GCC, which access
       the source code directory on the fly, and make it unsafe to modify the
       sources during the compilation process. It
       is safe to modify the source code after the compiler completes input
       aggregation.
2. **Abstract Syntax Tree (AST)**: The AST is the output of parsing the
   build sources. The qxc compiler does not have a traditional lexing stage, as
   the parser
   uses a handwritten iterative recursive descent parser that completes the
   logical equivalent of lexing during the parsing step on the fly. This was
   done to avoid
   having to allocate memory for lexing and improve the speed of the
   compiler.
3. **Qux Virtual Machine Intermediate Representation (QVMIR)**: The QVMIR is
   the
   output of the
   semantic analysis. It is a mixed level IR consisting of expressions,
   mutations, and procedure calls. QVMIR is the first stage that is machine
   dependent. Conversion from the AST to the QVMIR is where most the of
   compiler's code is
   focused. QVMIR is also the format used for the constexpr virtual machine
   evaluator.

### Backend Compilation Layers

The compiler is designed to support multiple backends, however the only
backend currently supported is LLVM.

#### LLVM Backend

There are two middle pathways in the LLVM backend:

##### LLVM IR Pathway

1. **LLVM Intermediate Representation (LLVM IR)**: The LLVM IR is the output of
   the VMIR to LLVM IR conversion for normal function. This conversion is
   handled by the LLVM
   Code Generator. It is immediately converted by the LLVM code generator to
   machine code and does not persist.
2. **Object Code**: The object code is the output of the LLVM IR to
   object code conversion using LLVM's machine code generation tools.
   This can either be output as object files (e.g. .o files on Linux) or as
   an independent link format.

##### LLVM Assembly Pathway

1. **Assembly Code**: Assembly code is extracted from the sources where it
   is used, then handled by LLVM's assembly parser. Assembly code is extracted
   from the AST or QVMIR (depending on the manner in which it is introduced, as
   an assembly procedure or as inline assembly) and then handed off to LLVM's
   assembler which converts it to object code. This conversion involves
   translation between Qux Assembly format and LLVM's Assembly format. Qux
   Assembly, for example, requires terminating semicolons for instructions and
   accepts `//` comments, while LLVM assembly does not require terminating
   semi-colons and does not accept `//` comments, instead using `;` comments.
   In Qux Assembly, all opcodes and registers are considered keywords and
   therefore UPPERCASE.
2. **Object Code**: In this instance, object code is the output of the
   assembler process.

### Linking

Linking can be done by LLVM or by an external linker.

#### LLVM Linking

1. **Object Code**: Object code is the output of either the LLVM IR or
   Assembly pathways. It is linked by LLVM's linking process to produce an
   executable binary.
2. **Executable Binary**: The executable binary is the output of the linking
   process.

## Resolver Architecture

The main method by which the compiler performs compilation is through the 
exercise of a resolver system. Abstractly, the resolver system consists of a 
set of resolvers, each of which can be viewed as a template for a question. 
The question can be invoked using parameters, and the resolver system solves 
the question for the answer. A question can require asking other questions, 
and questions can be processed in parallel. The resolver system provides 
semantic analysis, type checking, and QVMIR code generation. 

One of the goals of this system is to provide a manner to later perform 
incremental builds in a manner that can provide dependency tracking and 
cache validity checking. Incremental builds are not yet implemented, but the 
interface provides an API that allows this to be added later without 
requiring individual resolver implementations to be modified.

Fundamentally, a resolver works by taking inputs, then setting it's output. 
On the way, a resolver may query other resolvers, and the resolver system 
will trigger an error if a recursive query is detected. 

For example, consider a question like "What is the size of (X)?", this is a 
question that corresponds to a resolver. It might execute according to the 
following pseudocode flow:

```
ASK: What is the placement information of (::foo)?
  ASK: Is (::foo) a class, function, etc.? 
    ... omitted
    ANSWER: A class.
  ASK: What is the class placement information of (::foo)?
    ASK: What are the members of (::foo)?
      ... omitted
      ANSWER: (-> ::bar, ::baz, I32)
    ASK: What is the placement information of (-> ::bar)?
      ASK: Is (-> ::bar) a class, function, etc.? 
        ANSWER: A pointer
      ANSWER: size=8, align=8
    ASK: What is the placement information of (::bar)?
      ... omitted
      ANSWER: size=16, align=8
    ASK: What is the placement information of (I32)?
      ... omitted
      ANSWER: size=4, align=4
    CALCULATIONS:
      size=0, align=1,
      Add (-> ::bar) size=8, align=8
      size=8, align=8
      Add (::baz) size=16, align=8
      size=24, align=8
      Add (I32) size=4, align=4
      size=28, align=8
    ANSWER: size=28, align=8
  ANSWER: size=28, align=8  
```

This can actually be implemented mainly in one of two ways, the first is the 
older, dependency-return method, whereby the resolver uses "process". In this 
method, execution restarts when the resolver has an "unmet dependency". 
`get_dependency(...)` is used to attach a dependency, and `ready()` is used to 
check if all dependencies are met. If not, the resolver returns and the 
resolver engine looks for a resolver which is ready (has no unmet 
dependencies and is not resolved). `set_output(...)` is used to set the 
final output of the resolver, and `return` before `set_output` is called 
will leave the resolver in an unresolved state.


The second method is using coroutines, through the `co_process` interface. 
This works the same way conceptually, but instead of restarting the 
function's execution each time, coroutine frames pause and resume the frame 
at `co_await` points. This is computationally more efficient but 
`get_dependency` can still be used 
to attach multiple dependencies so that the compiler can gain more 
parallelism, as stopping each time a dependency is reached, even if we can 
discover more dependencies, does not provide the best parallelism. In the 
coroutine version, `co_return` is used in place of `set_value` and `return` 
cannot be used to leave the resolver in an unresolved state, with `co_await` 
being used to pause the resolver instead. The only function from the 
previous interface that remains 
useful is `get_dependency`.
     
