; My own imagining of how a possible emitted assembly program could look like
; by my compiler for the hirola-program.hir source code.

global _start

section .text

_start:

; All IR variables and named constants will have a pre-computed RSP offset.
; So here's a table of the offsets of the IR variables in hirola-program.
; RSP + (TOTAL_STACK_SPACE - OFFSET) is what's used to access a stack variable.
;
; OFFSETS:
;
; %const_0 :  1 * 8 =   8   OFFSET: 152
; %a_1     :  2 * 8 =  16   OFFSET: 144
; %const_1 :  3 * 8 =  24   OFFSET: 136
; %b_1     :  4 * 8 =  32   OFFSET: 128
; %const_2 :  5 * 8 =  40   OFFSET: 120
; %c_1     :  6 * 8 =  48   OFFSET: 112
; %c_2     :  7 * 8 =  56   OFFSET: 104
; %const_3 :  8 * 8 =  64   OFFSET: 96
; %a_2     :  9 * 8 =  72   OFFSET: 88
; %b_2     : 10 * 8 =  80   OFFSET: 80
; %d_1     : 11 * 8 =  88   OFFSET: 72
; %c_3     : 12 * 8 =  96   OFFSET: 64
; %a_3     : 13 * 8 = 104   OFFSET: 56
; %const_4 : 14 * 8 = 112   OFFSET: 48
; %_temp_0 : 15 * 8 = 120   OFFSET: 40
; %_temp_1 : 16 * 8 = 128   OFFSET: 32
; %_temp_2 : 17 * 8 = 136   OFFSET: 24
; %const_5 : 18 * 8 = 144   OFFSET: 16
; %_temp_3 : 19 * 8 = 152   OFFSET: 8
; %a_4     : 20 * 8 = 160   OFFSET: 0
;-------------------------
; TOTAL STACK SPACE: 160 B
;-------------------------
;
; Stack frame allocation algorithm:
;
; 1. Set TOTAL_STACK_SPACE = 0
; 2. Start at the END of the IR Instructions Directory.
;
; /* If it's an IR instruction that assigns to a new IR variable: */
;
; 3. IF IR_dir_entry->which_ir_instruction ==    IR_INSN_EQUATE or IR_INSN_ADD
;                                             or IR_INSN_SUB    or IR_INSN_MUL
;                                             or IR_INSN_DIV
;    THEN:
;
;           for(std::unordered_map<std::string, size_t>::iterator it: my_map)
;               it.second += 8;
;
;           my_map.emplace(insn.lhs_operand, 0);
;
; 4. Proceed to previous entry in IR Instructions Directory.
;
;
;
;
;
;
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
