[BITS 32]

GLOBAL check_A20
GLOBAL a20_bios
GLOBAl a20_in

SECTION .text
check_A20:
    pushad
    mov edi,0x112345  ;odd megabyte address.
    mov esi,0x012345  ;even megabyte address.
    mov [esi],esi     ;making sure that both addresses contain diffrent values.
    mov [edi],edi     ;(if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
    cmpsd             ;compare addresses to see if the're equivalent.
    popad
    jne A20_on        ;if not equivalent , A20 line is set.
    mov eax, 0
    ret               ;if equivalent , the A20 line is cleared.
A20_on:
    mov eax, 1
    ret

a20_bios:
    use16
    mov     ax,2403h                ;--- A20-Gate Support ---
    int     15h
    jb      a20_failed                  ;INT 15h is not supported
    cmp     ah,0
    jnz     a20_failed                  ;INT 15h is not supported
    
    mov     ax,2402h                ;--- A20-Gate Status ---
    int     15h
    jb      a20_failed              ;couldn't get status
    cmp     ah,0
    jnz     a20_failed              ;couldn't get status
    
    cmp     al,1
    jz      a20_activated           ;A20 is already activated
    
    mov     ax,2401h                ;--- A20-Gate Activate ---
    int     15h
    jb      a20_failed              ;couldn't activate the gate
    cmp     ah,0
    jnz     a20_failed              ;couldn't activate the gate
    jmp     a20_failed              ; if nothing worked, then don't assume A20 is activated

a20_in:
    in al, 0xee
    ret

a20_failed:
    mov eax, 0
    ret
 
a20_activated:                  ;go on
    mov eax, 1
    ret