/* Instruction Selection phase.
 *
 * Predefined pattern matching, from IR code to x64 Assembly language code, will
 * dictate what ASM instructions are emitted for a given Hirola source program.
 *
 * No cost analysis exists yet, so each IR code pattern has exactly one assembly
 * code pattern to be matched to. A basic first step toward optimizations like
 * this, where an IR code pattern has multiple assembly code patterns it can
 * be matched with, is using the lea instruction for faster multiplication by
 * 3, 5 and 9 that doesn't take up an ALU micro-op execution port.
 *
 * The generated assembly language program will be stored internally in a cache
 * locality-friendly contiguous Memory Arena and a Directory to navigate it,
 * much like how the AST and the SSA IR program are internally stored.
 */
class ASM_Code_Generation_Orchestrator_x64
{
public:
    /* Receives these from an IR Generation Orchestrator: */
    MEM_Arena IR_instructions_arena;
    IR_Instructions_Directory IR_instructions_dir;

    /* Brings into existence these new things: */
    MEM_Arena x64_ASM_instructions_arena;
    ASM_Instructions_Directory x64_ASM_instructions_dir;

    /* Receives this from the top-level compilation driver: */
    std::vector<std::vector<size_t>> ASM_code_generation_quotas;

    /* Constructor. */
    explicit ASM_Generation_Orchestrator
        (MEM_Arena&& IR_instructions_arena_in,
         IR_Instructions_Directory&& IR_instructions_dir_in,
         std::vector<std::vector<size_t>> ASM_code_generation_quotas_in)
    : IR_instructions_arena(std::move(IR_instructions_arena_in)),
      IR_instructions_dir(std::move(IR_instructions_dir_in)),
      x64_ASM_instructions_arena
        (MEM_Arena(std::string("x64 ASM Instructions Arena"))),
      x64_ASM_instructions_dir(ASM_Instructions_Directory(0, 0)),
      ASM_generation_quotas(std::move(ASM_code_generation_quotas_in)) {}

    uint8_t spawn_ASM_code_generator(std::vector<size_t> ASM_code_gen_quota);
};

class ASM_Code_Generator_x64
{
private:
    /* Receives these from an ASM Code Generation Orchestrator: */
    MEM_Arena* IR_instructions_arena;
    IR_Instructions_Directory* IR_instructions_dir;
    MEM_Areena* x64_ASM_instructions_arena;
    ASM_Instructions_Directory* x64_ASM_instructions_dir;
    std::vector<size_t> ASM_code_gen_quota;

public:
    /* IR variable names map to their current offset from RSP. */
    std::unordered_map<std::string, size_t> IR_vars_stack_offsets;

    size_t curr_func_total_stack_allocated;

    /* Constructor. */
    explicit ASM_Code_Generator_x64
        (MEM_Arena* IR_insn_arena_in,  IR_Instructions_Directory* ir_dir_in,
         MEM_Arena* x64_insn_arena_in, ASM_Instructions_Directory* asm_dir_in,
         std::vector<size_t> ASM_code_gen_quota_in)
    : IR_instructions_arena(IR_insn_arena_in), IR_instructions_dir(ir_dir_in),
      x64_ASM_instructions_arena(asm_insn_arena_in),
      x64_ASM_instructions_dir(asm_dir_in),
      ASM_code_gen_quota(ASM_code_gen_quota_in) {}

    uint8_t generate_ASM_code(void);

private:
    void setup_new_stack_frame(void);

    inline void match_IR_instruction_with_ASM_code_pattern(size_t IR_dir_entry);

    inline void emit_asm_for_equ_u64(size_t IR_dir_entry);
    inline void emit_asm_for_add_u64(size_t IR_dir_entry);
    inline void emit_asm_for_sub_u64(size_t IR_dir_entry);
    inline void emit_asm_for_mul_u64(size_t IR_dir_entry);
    inline void emit_asm_for_div_u64(size_t IR_dir_entry);
};

/* TODO: This only works because we still have only A SINGLE CODE BLOCK in the
 *       language. It will need to be called per Code Block in the function
 *       generate_ASM_code, when it finds the beginning of a Code Block of type
 *       FUNCTION or the MAIN_BLOCK. For now it will do, though.
 */
void ASM_Code_Generator_x64::setup_new_stack_frame(void)
{
    size_t   IR_insn_type;
    size_t   IR_insn_arena_offset;
    uint8_t* IR_insn_addr;
    std::unordered_map<std::string, size_t>::iterator it;

    /* Each IR instruction kind that assigns to a new IR variable. */
    union
    {
        ir_insn_div*    ir_insn_div_ptr;
        ir_insn_mul*    ir_insn_mul_ptr;
        ir_insn_sub*    ir_insn_sub_ptr;
        ir_insn_add*    ir_insn_add_ptr;
        ir_insn_equate* ir_insn_equ_ptr;
    };

    /* For each IR instruction that assigns to a new SSA IR variable:
     *   - Go over all IR variables already present on a stack frame slot,
     *     increase their offset from RSP by 8.
     *   - New IR variable's offset from RSP is 0.
     *   - Add 8 to TOTAL_STACK_ALLOCATED.
     */
    for(size_t i = 0; i < IR_instructions_dir->size(); ++i)
    {
        IR_insn_type = (*IR_instructions_dir)[i].which_ir_instruction;
        IR_insn_arean_offset = (*IR_instructions_dir)[i].ir_insn_arena_offset;
        IR_insn_addr = IR_instructions_arena->arena_ptr + IR_insn_arena_offset;

        if(   IR_insn_type == IR_INSN_EQUATE || IR_insn_type == IR_INSN_ADD
           || IR_insn_type == IR_INSN_SUB    || IR_insn_type == IR_INSN_MUL
           || IR_insn_type == IR_INSN_DIV) [[likely]]
        {
            curr_func_total_stack_allocated += 8;

            for(it: IR_vars_stack_offsets)
                it.second += 8;

            if(IR_insn_type == IR_INSN_EQUATE)
            {
                ir_insn_equ_ptr = (ir_insn_equate*)(IR_insn_addr);
                IR_vars_stack_offsets.emplace(ir_insn_equ_ptr->lhs, 0);
            }
            else if(IR_insn_type == IR_INSN_ADD)
            {
                ir_insn_add_ptr = (ir_insn_add*)(IR_insn_addr);
                IR_vars_stack_offsets.emplace(ir_insn_add_ptr->target, 0);
            }
            else if(IR_insn_type == IR_INSN_SUB)
            {
                ir_insn_sub_ptr = (ir_insn_sub*)(IR_insn_addr);
                IR_vars_stack_offsets.emplace(ir_insn_sub_ptr->target, 0);
            }
            else if(IR_insn_type == IR_INSN_MUL)
            {
                ir_insn_mul_ptr = (ir_insn_mul*)(IR_insn_addr);
                IR_vars_stack_offsets.emplace(ir_insn_mul_ptr->target, 0);
            }
            else if(IR_insn_type == IR_INSN_DIV)
            {
                ir_insn_div_ptr = (ir_insn_div*)(IR_insn_addr);
                IR_vars_stack_offsets.emplace(ir_insn_div_ptr->quotient, 0);
            }
        }
    }
    return;
}

/* Go over the handed quota of Code Blocks, find all their IR Instructions in
 * the IR Instructions Directory, analyze each instruction and emit the proper
 * assembly language instructions for it.
 */
uint8_t ASM_Code_Generator_x64::generate_ASM_code(void)
{
    size_t  code_block_ix;
    size_t  i;
    size_t  j;
    size_t  arena_offset;
    uint8_t ret = 0;
    x64_Assembly_Instruction* asm_insn;

    this->setup_new_stack_frame();

    /* Emit assembly instructions that set up the main stack frame:
     *
     * sub rsp, curr_func_total_stack_allocated
     *
     * TODO: This only works when we have a single ASM code generator spawned
     *       by the ASM code generation orchestrator to emit the assemblt code
     *       for ALL the Code Blocks in one pass here. Also it's a little bit of
     *       a hack that we say this assembly instruction belongs to code block
     *       0, statement 0, IR Instruction 0 because it really doesn't belong
     *       to any source code statement or IR instruction... Any later passes
     *       over the ASM Instructions Directory will think this ASM instruction
     *       belongs to statement 0, IR Instruction 0 ... Find a fix!
     *
     *       OR perhaps have a rule that the very first assembly instruction
     *       of each code block marked as FUNCTION or the MAIN_BLOCK is not
     *       at all emitted 'for a given source statement or IR instruction',
     *       but is always the stack space reserving instruction.
     *
     *       AND this code needs to be in the loop that found the required
     *       Code Block in the IR Instructions Directory. The code should check
     *       if it's a FUNCTION BLOCK or the MAIN BLOCK, then emit this assembly
     *       instruction here!
     *
     *       Temporarily, for now, it will do though.
     */
    arena_offset = x64_ASM_instructions_arena->add_entry
                       <x64_Assembly_Instruction>(x64_insn_sub_ix, 1);

    asm_insn = (x64_Assembly_Instruction*)
                   (x64_ASM_instructions_arena->arena_ptr + arena_offset);

    asm_insn->insn_operands[0] = x64_ASM_Instruction_Operand
     (x64_operand_type_immediate, 0, 0, curr_func_total_stack_allocated, 0, "");

    x64_ASM_instructions_dir->emplace_back(0, 0, 0
        x64_ASM_instructions_dir->size(), arena_offset);

    /* Find the code blocks in the IR Instructions Directory. */
    for(i = 0; i < ASM_code_gen_quota.size(); ++i)
    {
        for(j = 0; j < IR_instructions_dir->size(); ++j)
            if((*IR_instructions_dir)[j].code_block_ix == ASM_code_gen_quota[i])
                break;

        /* Code block not found. */
        if(j == IR_instructions_dir->size())
        {
            std::cout << "\n\n*** [ERR] Internal Compiler Error! *** \n\n"
            "Emitting x64 assembly code for IR instructions of\nCode Block[" <<i
            << "] but block wasn't found in the IR Instructions Directory.\n\n";
            std::abort();
        }

        /* Found the 1st IR Instruction of the i-th Code Block: IR_Dir[j] */
        while(j != IR_instructions_dir->size() && j == ASM_code_gen_quota[i])
        {
            /* Analyze the IR Instruction and emit assembly code for it. */
            match_IR_instruction_with_ASM_code_pattern(j++);
        }
    }
    return ret;
}

/* Analyze the input IR Instruction, match it with a known IR code pattern and
 * dispatch it to its respective Assembly code generation function.
 */
void ASM_Code_Generator_x64::match_IR_instruction_with_ASM_code_pattern
                                                          (size_t IR_insn_entry)
{
    size_t insn_type =
                     (*IR_instructions_dir)[IR_insn_entry].which_ir_instruction;

    switch(insn_type)
    {
    case IR_INSN_EQUATE:
    {
        emit_asm_for_equ_u64(IR_insn_entry);
        break;
    }
    case IR_INSN_ADD:
    {
        emit_asm_for_add_u64(IR_insn_entry);
        break;
    }
    case IR_INSN_SUB:
    {
        emit_asm_for_sub_u64(IR_insn_entry);
        break;
    }
    case IR_INSN_MUL:
    {
        emit_asm_for_mul_u64(IR_insn_entry);
        break;
    }
    case IR_INSN_DIV:
    {
        emit_asm_for_div_u64(IR_insn_entry);
        break;
    }
    default:
    {
        std::cout << "\n\n*** [ERR] Internal Compiler Error ***\n\n
        x64 assembly code generation: Unknown IR instruction type index: "
        << insn_type << "\nKnown IR instruction types go from 0 to "
        << TOTAL_IR_INSN_TYPES - 1 << "\n\n";
        std::abort();
    }  /* End last case. */
    }; /* End switch.    */

}

/*--------------------------------------------------------------------------|
 * Pattern 1:   { %const_0 = 5 }     --->   IR_Variable1 = literal uint64_t.|
 *                                           -------------------------------|
 * 1. Store the literal in register rbp.     |  mov rbp, 5                  |
 * 2. Store register rbp in [rsp + offset].  |  mov [rsp + offset1], rbp    |
 *--------------------------------------------------------------------------|
 * Pattern 2:   { %a_1 = %const_0 }  --->   IR_Variable_1 = IR_Variable_2.  |
 *                                           -------------------------------|
 * 1. Store var_2 in rbp from stack offset.  |  mov rbp, [rsp + offset2]    |
 * 2. Store register rbp in its stack slot.  |  mov [rsp + offset1], rbp    |
 *--------------------------------------------------------------------------|
 */
void ASM_Code_Generator_x64::emit_asm_for_equ_u64(size_t IR_dir_entry)
{
    size_t arena_offset;
    x64_Assembly_Instruction* asm_insn;
    ir_insn_equate IR_insn;
    std::string equ_rhs_string;
    std::string equ_lhs_string;
    size_t code_block_ix = (*IR_instructions_dir)[IR_dir_entry].code_block_ix;
    size_t statement_ix  = (*IR_instructions_dir)[IR_dir_entry].statement_ix;
    size_t ir_insn_ix    = (*IR_instructions_dir)[IR_dir_entry].ir_insn_ix;

    arena_offset = (*IR_instructions_dir)[IR_dir_entry].ir_insn_arena_offset;

    IR_insn = (ir_insn_equate*)
                  (IR_instructions_arena->arena_ptr + arena_offset);

    equ_rhs_string = IR_insn->rhs;
    equ_lhs_string = IR_insn->lhs;

    /* The first character of an IR operand is either alphabetical, which means
     * it's a variable name, or numeric, which means a literal.
     */

    /* RHS is a u64 Literal => Pattern 1. */
    if(is_digit(equ_rhs_string[0]))
    {
        /* Emit:  mov rbp, <equ_rhs_u64_literal> */
        arena_offset = x64_ASM_instructions_arena->add_entry
                        <x64_Assembly_Instruction>(x64_insn_mov_ix, 2);

        asm_insn = (x64_Assembly_Instruction*)
                       (x64_ASM_instructions_arena->arena_ptr + arena_offset);

        asm_insn->insn_operands[0] = x64_ASM_Instruction_Operand
                            (x64_operand_type_reg, x64_reg_rbp_ix, 0, 0, 0, "");

        asm_insn->insn_operands[1] = x64_ASM_Instruction_Operand
         (x64_operand_type_immediate, 0, 0, std::stoull(equ_rhs_string), 0, "");
    }
    /* RHS is a variable name => Pattern 2. */
    else
    {
        /* Emit: mov rbp, [rsp + computed_stack_offset_rhs_IR_var] */
        arena_offset = x64_ASM_instructions_arena->add_entry
                        <x64_Assembly_Instruction>(x64_insn_mov_ix, 2);

        asm_insn = (x64_Assembly_Instruction*)
                       (x64_ASM_instructions_arena->arena_ptr + arena_offset);

        asm_insn->insn_operands[0] = x64_ASM_Instruction_Operand
                            (x64_operand_type_reg, x64_reg_rbp_ix, 0, 0, 0, "");

        asm_insn->insn_operands[1] = x64_ASM_Instruction_Operand
                 (x64_operand_type_reg_plus_immediate_as_ptr, x64_reg_rsp_ix, 0,
                  IR_vars_stack_offsets[equ_rhs_string], 0, "");
    }

    /* Store 1st ASM instruction of pattern1/2 in ASM Instructions Directory. */
    x64_ASM_instructions_dir->emplace_back(code_block_ix, statement_ix,
                    ir_insn_ix, x64_ASM_instructions_dir->size(), arena_offset);

    /* 2nd ASM instruction of pattern1/2 is the same.
     * Emit: mov [rsp + computed_stack_offset_lhs_IR_var], rbp
     */
    arena_offset = x64_ASM_instructions_arena->add_entry
                    <x64_Assembly_Instruction>(x64_insn_mov_ix, 2);

    asm_insn = (x64_Assembly_Instruction*)
                   (x64_ASM_instructions_arena->arena_ptr + arena_offset);

    asm_insn->insn_operands[0] = x64_ASM_Instruction_Operand
                 (x64_operand_type_reg_plus_immediate_as_ptr, x64_reg_rsp_ix, 0,
                  IR_vars_stack_offsets[equ_lhs_string], 0, "");

    asm_insn->insn_operands[1] = x64_ASM_Instruction_Operand
                            (x64_operand_type_reg, x64_reg_rbp_ix, 0, 0, 0, "");

    /* Store 2nd ASM instruction of pattern1/2 in ASM Instructions Directory. */
    x64_ASM_instructions_dir->emplace_back(code_block_ix, statement_ix,
                    ir_insn_ix, x64_ASM_instructions_dir->size(), arena_offset);

    return;
}

/*----------------------------------------------------------------------------|
 * Pattern 3:   { %c_3 = %a_2 + %const_0 } --->  IR_var1 = IR_var2 + IR_var3. |
 *                                               -----------------------------|
 * 1. Store var2 in rbp from its stack offset.   |  mov rbp, [rsp + offset2]  |
 * 2. Store var3 in rbx from its stack offset.   |  mov rbx, [rsp + offset3]  |
 * 3. Add rbx into rbp.                          |  add rbp, rbx              |
 * 4. Store register rbp in var1's stack offset. |  mov [rsp + offset1], rbp  |
 *----------------------------------------------------------------------------|
 */
void ASM_Code_Generator_x64::emit_asm_for_add_u64(size_t IR_dir_entry)
{

    return;
}

/*----------------------------------------------------------------------------|
 * Pattern 4:   { %_temp_1 = %a_2 - %a_3 }  --->  IR_var1 = IR_var2 - IR_var3 |
 *                                               -----------------------------|
 * 1. Store var2 in rbp from its stack offset.   |  mov rbp, [rsp + offset2]  |
 * 2. Store var3 in rbx from its stack offset.   |  mov rbx, [rsp + offset3]  |
 * 3. Subtract rbx from rbp, put result in rbp.  |  sub rbp, rbx              |
 * 4. Store register rbp in var1's stack offset. |  mov [rsp + offset1], rbp  |
 *----------------------------------------------------------------------------|
 */
void ASM_Code_Generator_x64::emit_asm_for_sub_u64(size_t IR_dir_entry)
{

    return;
}

/*----------------------------------------------------------------------------|
 * Pattern 5:   { %_temp_1 = %a_2 * %a_3 }  --->  IR_var1 = IR_var2 * IR_var3 |
 *                                               -----------------------------|
 * 1. Store var2 in rbp from its stack offset.   |  mov rbp, [rsp + offset2]  |
 * 2. Store var3 in rax from its stack offset.   |  mov rax, [rsp + offset3]  |
 * 3. Multiply. RAX * Operand2 = RDX:RAX <-- low |  mul rax, rbp              |
 * 4. Store register rax in var1's stack offset. |  mov [rsp + offset1], rax  |
 *----------------------------------------------------------------------------|
 * NOTE: This is UNSIGNED multiplication. If the result needs to use more     |
 *       than 64 bits, the higher bits go to RDX and the overflowed low bits  |
 *       are in RAX. The first operand is always RAX, second one - we pick.   |
 *----------------------------------------------------------------------------|
 */
void ASM_Code_Generator_x64::emit_asm_for_mul_u64(size_t IR_dir_entry)
{

    return;
}

/*----------------------------------------------------------------------------|
 * Pattern 6:   { %_temp_1 = %a_2 / %a_3 }  --->  IR_var1 = IR_var2 / IR_var3 |
 *                                                ----------------------------|
 * 1. Store var2 in RAX from its stack offset.    | mov rax, [rsp + offset2]  |
 * 2. Sign-extend RAX into RDX to run division.   | cqo                       |
 * 3. Store var3 in rbp from its stack offset.    | mov rbp, [rsp + offset3]  |
 * 4. Divide RDX:RAX / Operand2 = RDX:RAX <-- low | div rbp                   |
 * 5. Store register rax in var1's stack offset.  | mov [rsp + offset1], rax  |
 *----------------------------------------------------------------------------|
 * NOTE: This is UNSIGNED division. The quotient goes in RAX, the remainder   |
 *       goes in RDX, which is why we sign-extend RAX into RDX beforehand.    |
 *       What we divide is in RAX. What we divide by is in a register or a    |
 *       memory location of our choosing.                                     |
 *----------------------------------------------------------------------------|
 */
void ASM_Code_Generator_x64::emit_asm_for_div_u64(size_t IR_dir_entry)
{

    return;
}
