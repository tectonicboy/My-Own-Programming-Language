/* The Instruction Selection phase.
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
class Instruction_Selector_x64
{
    MEM_Arena x64_ASM_Instructions_Arena;
    ASM_Instructions_Directory x64_ASM_Instructions_Dir;
    size_t total_asm_insns_emitted;





}
