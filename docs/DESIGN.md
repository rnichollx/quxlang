# RyanScript1031 (to be renamed)

## Data Layers

There are 3 layers to the translation process:

### Abstract Syntax Tree (AST)
  
The abstract syntax tree is a representation of the syntax as parsed. It does not have any semantic knowledge.

### Semantic Syntax Tree (SST)

The semantic syntax tree is an intermediate structure that resolves cross references to symbols, types, etc.

### Language Intermediate Representation (LIR)

The language intermediate representation is the last backend independent representation of the code. The LIR is 
intended to provide backend independence (LLVM, JVM, etc.).

### LLVM IR
  
Currently, LLVM is the only code generator available.

## Code Layers

### AST Parser
  
The AST Parser parses the code into the abstract syntax tree. Parsing and lexing are not implemented as 
separate stages, rather the AST Parser is a single pass parser. 

### Semantic Analyzer

The semantic analyzer generates cross links between symbols, types, etc, and does some basic sanity checks.

### LIR Generator

The LIR generator generates the language intermediate representation from the semantic syntax tree.