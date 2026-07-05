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

# Pointer arithmetic

If creating an array pointer, it is legal to create a one-past-the-end pointer.
Creating an array pointer to anything before the start of the array or after the one
past the end pointer produces an _illegal-pointer-value_.

Comparing an `illegal-pointer-value` to any other pointer or dereferencing it
triggers undefined behavior.

Pointer arithmetic with an _illegal pointer value_ produces an _illegal pointer value_,
or _illegal pointer offset_, even if the arithmetic would cancel out.

An _illegal pointer offset_ is a value of type PTRDIFF which if added or subtracted to
any pointer, produces an _illegal pointer value_.

If a pointer is _invalidated_, pointer arithmetic remains valid under the original
range of valid values, but dereferencing the invalid pointer is not.

Invalidated pointers are not _illegal pointer values_, they may continue to be
compared with other pointers, but not dereferenced. Dereferencing an invalidated
pointer is undefined behavior.

Casting an _illegal pointer value_ or illegal pointer offset where supported to
UINTPTR or SINTPTR produces an unspecified integer. Casting an illegal pointer 
offset to SZ, UINTPTR etc produces an unspecified integer.

Casting the same illegal pointer value or illegal pointer offset to an integer 
twice is not required to produce the same value each time.
 

