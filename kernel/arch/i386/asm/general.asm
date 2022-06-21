[BITS 32]

global checkProtected

SECTION .text

checkProtected:
    smsw eax
    and eax, 0x1
    cmp eax, 1
    je inProtected
    mov eax, 0
    ret
inProtected:
    mov eax, 1
    ret