; My own imagining of how a possible emitted assembly program could look like
; by my compiler for the hirola-program.hir source code.

global _start

section .text

_start:

; All IR variables and named constants will have a pre-computed RSP offset.
; So here's a table of the offsets of the IR variables in hirola-program:

; %const_0 :  1 * 8 =   8
; %a_1     :  2 * 8 =  16
; %const_1 :  3 * 8 =  24
; %b_1     :  4 * 8 =  32
; %const_2 :  5 * 8 =  40
; %c_1     :  6 * 8 =  48
; %c_2     :  7 * 8 =  56
; %const_3 :  8 * 8 =  64
; %a_2     :  9 * 8 =  72
; %b_2     : 10 * 8 =  80
; %d_1     : 11 * 8 =  88
; %c_3     : 12 * 8 =  96
; %a_3     : 13 * 8 = 104
; %const_4 : 14 * 8 = 112
; %_temp_0 : 15 * 8 = 120
; %_temp_1 : 16 * 8 = 128
; %_temp_2 : 17 * 8 = 136
; %const_5 : 18 * 8 = 144
; %_temp_3 : 19 * 8 = 152
; %a_4     : 20 * 8 = 160
;-------------------------
; TOTAL STACK SPACE: 160 B
;-------------------------

sub rsp, 160

; %const_0 = 5
mov rbx, 5
mov [rsp + (160 - 8)], rbx  ; rsp + (total_for_stack_frame - this_var_offset)

; %a_1 = %const_0
mov rbx, [rsp + (160 - 8)]
mov [rsp + (160 - 16)], rbx




; FINISH
mov rdi, [rsp-<stack_offset_exit_code_var>]  ; exit_group()'s arg. (exit code)
mov rax, 60                                  ; eixt_group()'s syscall number.
syscall                                      ; run the syscall.
