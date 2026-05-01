# Optimizations

## Interfaces

It's possible when compiling to machine code, if the whole program is known, to identify all places some function 
pointer table is stored into an interface table. If that occurs, further calls to that interface can be optimized into
an enum value which is then binary searched inline, instead of doing a dynamic dispatch through the interface table. 

This allows potential inlining of the various call targets at the call sites of the interface values, and also means
if an interface has only 1 implementor, it can be directly inlined at every call site.

This is possible when doing monolithic compilation, where all code is compiled together and can be seen as a form of
link-time optimization. It would not provide a useful interface for separately compiled plugins.