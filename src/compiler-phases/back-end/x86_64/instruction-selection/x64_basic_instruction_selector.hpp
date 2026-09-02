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

    /* One function per IR->ASM code pattern match? */

    /* IR --> ASM  CODE PATTERNS
     *
     *==========================================================================
     *==========================================================================
     *
     * Pattern 1:   { %const_0 = 5 }   --->   IR_Variable = literal uint64_t.
     *
     * STEPS:
     *
     * 1. Store the literal in register rbp.    |  mov rbp, 5
     * 3. Store register rbp in [rsp + offset]. |  mov [rsp], rbp
     *--------------------------------------------------------------------------
     *
     *==========================================================================
     *==========================================================================
     *
     * Pattern 2:   { %a_1 = %const_0 }   --->   IR_Variable_1 = IR_Variable_2.
     *
     * STEPS:
     *
     * 1. Store var_2 in rbp from stack offset.  | mov rbp, [rsp + offset2]
     * 3. Store register rbp in its stack slot.  | mov [rsp + offset1], rbp
     *--------------------------------------------------------------------------
     *
     * Pattern 3:   { %c_3 = %a_2 + %const_0 } ---> IR_var1 = IR_var2 + IR_var3.
     *
     * STEPS:
     *
     * 1. Store var2 in rbp from its stack offset.   | mov rbp, [rsp + offset2]
     * 2. Store var3 in rbx from its stack offset.   | mov rbx, [rsp + offset3]
     * 3. Add rbx into rbp.                          | add rbp, rbx
     * 4. Store register rbp in var1's stack offset. | mov [rsp + offset1], rbp
     *--------------------------------------------------------------------------
     *
     */

};

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
     *   - New IR variable's offset from RSP is current TOTAL_STACK_ALLOCATED.
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
