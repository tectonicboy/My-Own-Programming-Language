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

    /* One function per IR->ASM code pattern match? */


};
