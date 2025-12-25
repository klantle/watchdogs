    .text
    .global asm_nothing

#ifdef __LINUX__
    .type asm_nothing, @function
#endif

asm_nothing:
    ret
