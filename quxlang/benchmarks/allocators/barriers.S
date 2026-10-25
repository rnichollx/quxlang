/* Opaque compiler barriers matching the Quxlang ASM_PROCEDURE calls. */
#if defined(__APPLE__)
#define SYMBOL(name) _##name
#else
#define SYMBOL(name) name
#endif


.text
.p2align 2
.globl SYMBOL(DO_NOT_OPTIMIZE)
SYMBOL(DO_NOT_OPTIMIZE):
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_WRITE_BYTE)
SYMBOL(ALLOCATION_WRITE_BYTE):
#if defined(__aarch64__)
    mov w1, #1
    strb w1, [x0]
#elif defined(__x86_64__) && defined(__linux__)
    movb $1, (%rdi)
#else
#error Unsupported allocator benchmark host
#endif
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_WRITE_WORD)
SYMBOL(ALLOCATION_WRITE_WORD):
#if defined(__aarch64__)
    str x1, [x0]
#elif defined(__x86_64__) && defined(__linux__)
    movq %rsi, (%rdi)
#else
#error Unsupported allocator benchmark host
#endif
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_TICKS)
SYMBOL(ALLOCATION_TICKS):
#if defined(__aarch64__)
    isb
    mrs x0, cntvct_el0
    isb
#elif defined(__x86_64__) && defined(__linux__)
    subq $16, %rsp
    movl $1, %edi
    movq %rsp, %rsi
    movl $228, %eax
    syscall
    movq (%rsp), %rax
    imulq $1000000000, %rax
    addq 8(%rsp), %rax
    addq $16, %rsp
#endif
    ret

.p2align 2
.globl SYMBOL(ALLOCATION_FREQUENCY)
SYMBOL(ALLOCATION_FREQUENCY):
#if defined(__aarch64__)
    mrs x0, cntfrq_el0
#elif defined(__x86_64__) && defined(__linux__)
    movl $1000000000, %eax
#endif
    ret

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
