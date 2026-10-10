/* Opaque compiler barriers matching the Quxlang ASM_PROCEDURE calls. */
#if defined(__APPLE__)
#define SYMBOL(name) _##name
#else
#define SYMBOL(name) name
#endif

#if !defined(__aarch64__)
#error The allocator benchmark timer requires ARM64
#endif

.text
.p2align 2
.globl SYMBOL(DO_NOT_OPTIMIZE)
SYMBOL(DO_NOT_OPTIMIZE):
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_TICKS)
SYMBOL(ALLOCATION_TICKS):
    isb
    mrs x0, cntvct_el0
    isb
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_FREQUENCY)
SYMBOL(ALLOCATION_FREQUENCY):
    mrs x0, cntfrq_el0
    ret

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
