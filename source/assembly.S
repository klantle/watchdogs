/* source/assembly.S */

#if defined(__ANDROID__)
/* AArch64 (Termux / Android) */
    .text
    .global asm_cross_clear_regs
    .type asm_cross_clear_regs, %function
asm_cross_clear_regs:
    mov x0, #0
    mov x1, #0
    ret

#elif defined(__LINUX__)
/* x86_64 SysV ABI (Linux) */
    .text
    .global asm_cross_clear_regs
    .type asm_cross_clear_regs, @function
asm_cross_clear_regs:
    xor %eax, %eax
    xor %ecx, %ecx
    ret

#elif defined(__WINDOWS32__)
/* x86_64 Microsoft ABI (Windows / MinGW / MSYS2) */
    .text
    .global asm_cross_clear_regs
    .def asm_cross_clear_regs; .scl 2; .type 32; .endef
asm_cross_clear_regs:
    xor %eax, %eax
    xor %ecx, %ecx
    ret
#endif
