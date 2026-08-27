;===============================================================================
; !!!!!!!!!!!!   WARNING WARNING WARNING WARNING   !!!!!!!!!!
;
; LLM-generated assembly code file, for personal practice and learning purposes
; only, not a part of the compiler's operation or of any further subtabce to the
; project whatsoever.
;
; Output ( echo $? ) after running it: 193.
;===============================================================================

; Standalone Hirola program, NASM Intel syntax, no libc, no external linkage.
;
; Assemble: nasm -f elf64 hirola-program.asm -o hirola-program.o
; Link    : ld hirola-program.o -o hirola-program
; Run     : ./hirola-program ; echo $?
;
; _start is the one symbol that MUST stay global -- see note at the bottom.

global  _start

section .text

_start:
    mov     rbp, rsp        ; establish a frame base (no push: no caller to preserve)
    sub     rsp, 160        ; 20 SSA values * 8 bytes, one stack slot each

    ; %const_0 = 5
    mov     rax, 5
    mov     [rbp-8], rax            ; const_0

    ; %a_1 = %const_0
    mov     rax, [rbp-8]
    mov     [rbp-16], rax           ; a_1

    ; %const_1 = 0
    mov     rax, 0
    mov     [rbp-24], rax           ; const_1

    ; %b_1 = %const_1
    mov     rax, [rbp-24]
    mov     [rbp-32], rax           ; b_1

    ; %const_2 = 9444
    mov     rax, 9444
    mov     [rbp-40], rax           ; const_2

    ; %c_1 = %const_2
    mov     rax, [rbp-40]
    mov     [rbp-48], rax           ; c_1

    ; %c_2 = %a_1
    mov     rax, [rbp-16]
    mov     [rbp-56], rax           ; c_2

    ; %const_3 = 7
    mov     rax, 7
    mov     [rbp-64], rax           ; const_3

    ; %a_2 = %const_3
    mov     rax, [rbp-64]
    mov     [rbp-72], rax           ; a_2

    ; %b_2 = %a_2
    mov     rax, [rbp-72]
    mov     [rbp-80], rax           ; b_2

    ; %d_1 = %c_2
    mov     rax, [rbp-56]
    mov     [rbp-88], rax           ; d_1

    ; %c_3 = %a_2 + %const_0
    mov     rax, [rbp-72]           ; a_2
    mov     rcx, [rbp-8]            ; const_0
    add     rax, rcx
    mov     [rbp-96], rax           ; c_3

    ; %a_3 = %a_2 + %b_2
    mov     rax, [rbp-72]           ; a_2
    mov     rcx, [rbp-80]           ; b_2
    add     rax, rcx
    mov     [rbp-104], rax          ; a_3

    ; %const_4 = 10
    mov     rax, 10
    mov     [rbp-112], rax          ; const_4

    ; %_temp_0 = %const_4 + %d_1
    mov     rax, [rbp-112]          ; const_4
    mov     rcx, [rbp-88]           ; d_1
    add     rax, rcx
    mov     [rbp-120], rax          ; _temp_0

    ; %_temp_1 = %a_3 * %c_3
    mov     rax, [rbp-104]          ; a_3
    mov     rcx, [rbp-96]           ; c_3
    imul    rax, rcx
    mov     [rbp-128], rax          ; _temp_1

    ; %_temp_2 = %_temp_0 / %_temp_1
    mov     rax, [rbp-120]          ; _temp_0 (dividend, low half)
    cqo                              ; sign-extend rax into rdx:rax
    mov     rcx, [rbp-128]          ; _temp_1 (divisor)
    idiv    rcx                      ; quotient -> rax, remainder -> rdx (unused)
    mov     [rbp-136], rax          ; _temp_2

    ; %const_5 = 77
    mov     rax, 77
    mov     [rbp-144], rax          ; const_5

    ; %_temp_3 = %a_3 - %const_5
    mov     rax, [rbp-104]          ; a_3
    mov     rcx, [rbp-144]          ; const_5
    sub     rax, rcx
    mov     [rbp-152], rax          ; _temp_3

    ; %a_4 = %_temp_2 + %_temp_3
    mov     rax, [rbp-136]          ; _temp_2
    mov     rcx, [rbp-152]          ; _temp_3
    add     rax, rcx
    mov     [rbp-160], rax          ; a_4

    ; process exit: sys_exit(a_4)
    mov     rdi, [rbp-160]          ; exit_group()'s status argument
    mov     rax, 60                 ; syscall number for exit_group
    syscall
