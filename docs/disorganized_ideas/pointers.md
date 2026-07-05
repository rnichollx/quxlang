# pointer like types


Quxlang has several pointer adjacent types:

SZ / SIZE - An integer large enough to store the size of any memory region

PTRDIFF / OFFSET - An integer large enough to store the signed offset between any
 two valid memory locations in the same allocation.
 
PTRDIFF is normally the signed variant of SZ, _but not necessarily_. On 16-bit or 32-bit
platforms where the entire memory space is consumed by one alloc, it might be possible
to have a single memory alloc which consumes the entire address space. In such instances 
it may be N+1 or N*2 bit for example.

On standard x64 processors, this scenario is not possible and ISZ can be the same size
as SZ.
 

