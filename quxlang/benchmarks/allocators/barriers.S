/* Opaque compiler barriers matching the Quxlang ASM_PROCEDURE calls. */
#if defined(__APPLE__)
#define SYMBOL(name) _##name
#else
#define SYMBOL(name) name
#endif

#if !defined(__aarch64__) && !defined(__x86_64__)
#error Unsupported benchmark barrier architecture
#endif

.text
.p2align 2
.globl SYMBOL(DO_NOT_OPTIMIZE)
SYMBOL(DO_NOT_OPTIMIZE):
    ret

.p2align 2
.globl SYMBOL(CLOBBER_MEMORY)
SYMBOL(CLOBBER_MEMORY):
    ret

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
