/* The basic machine description for x86_64 CPUs that is used to emit basic x64
 * assembly language programs that are capable of running on a vast number
 * of existing machines. To be assembled using GNU Assembler.
 *
 * Contains a minimal set of what the x86_64 architecture has contained for
 * over a decade - common assembly instructions, registers and ABI rules that
 * are enforced during Instruction Selection.
 *
 * As an early optimization, it was decided to omit the rbp register by default,
 * enabling it to be used freely as an extra general purpose 64-bit register.
 *
 * Defined herein are lookup tables for basic assembly instruction and register
 * names, the Assembly_Instruction class, the ASM_Instruction_Operand class,
 * x64 GNU/Linux SysV ABI conventions and the architectural machine word size.
 *
 * The reason we have named indices into the lookup tables is that it makes code
 * using these tables more readable. Instead of having to write table_regs[2],
 * we can write table_regs[reg_rbx]. The strings in the tables themselves are
 * used to emit the actual correct assembly language program, from the internal
 * representation of the same (a Memory Arena and a Directory - the same
 * internal compiler representation used for the AST and the SSA IR program).
 */

/*----------------------------------------------------------------------------*/

/* Lookup table with named indices of basic x64 assembly instruction names. */

constexpr size_t x64_basicMD_supported_asm_insns_arity = 6;

constexpr size_t x64_insn_mov_ix     = 0;
constexpr size_t x64_insn_add_ix     = 1;
constexpr size_t x64_insn_sub_ix     = 2;
constexpr size_t x64_insn_mul_ix     = 3;
constexpr size_t x64_insn_div_ix     = 4;
constexpr size_t x64_insn_syscall_ix = 5;

constexpr std::array<const char*, x64_basicMD_supported_asm_insns_arity>
lookup_table_x64_insn_names =
{
    "mov",
    "add",
    "sub",
    "mul",
    "div",
    "syscall"
};

/*----------------------------------------------------------------------------*/

/* Lookup table with named indices of basic x64 architectural register names. */

constexpr size_t x64_basicMD_supported_asm_regs_arity = 32;

constexpr size_t x64_reg_rax_ix   = 0;
constexpr size_t x64_reg_rbx_ix   = 1;
constexpr size_t x64_reg_rcx_ix   = 2;
constexpr size_t x64_reg_rdx_ix   = 3;

constexpr size_t x64_reg_r8_ix    = 4;
constexpr size_t x64_reg_r9_ix    = 5;
constexpr size_t x64_reg_r10_ix   = 6;
constexpr size_t x64_reg_r11_ix   = 7;
constexpr size_t x64_reg_r12_ix   = 8;
constexpr size_t x64_reg_r13_ix   = 9;
constexpr size_t x64_reg_r14_ix   = 10;
constexpr size_t x64_reg_r15_ix   = 11;

constexpr size_t x64_reg_rdi_ix   = 12;
constexpr size_t x64_reg_rsi_ix   = 13;

constexpr size_t x64_reg_rsp_ix   = 14;
constexpr size_t x64_reg_rbp_ix   = 15;

constexpr size_t x64_reg_xmm0_ix  = 16;
constexpr size_t x64_reg_xmm1_ix  = 17;
constexpr size_t x64_reg_xmm2_ix  = 18;
constexpr size_t x64_reg_xmm3_ix  = 19;
constexpr size_t x64_reg_xmm4_ix  = 20;
constexpr size_t x64_reg_xmm5_ix  = 21;
constexpr size_t x64_reg_xmm6_ix  = 22;
constexpr size_t x64_reg_xmm7_ix  = 23;
constexpr size_t x64_reg_xmm8_ix  = 24;
constexpr size_t x64_reg_xmm9_ix  = 25;
constexpr size_t x64_reg_xmm10_ix = 26;
constexpr size_t x64_reg_xmm11_ix = 27;
constexpr size_t x64_reg_xmm12_ix = 28;
constexpr size_t x64_reg_xmm13_ix = 29;
constexpr size_t x64_reg_xmm14_ix = 30;
constexpr size_t x64_reg_xmm15_ix = 31;

constexpr std::array<const char*, x64_basicMD_supported_asm_regs_arity>
lookup_table_x64_reg_names =
{
    "rax",
    "rbx",
    "rcx",
    "rdx",

    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",

    "rdi",
    "rsi",

    "rsp",
    "rbp",

    "xmm0",
    "xmm1",
    "xmm2",
    "xmm3",
    "xmm4",
    "xmm5",
    "xmm6",
    "xmm7",
    "xmm8",
    "xmm9",
    "xmm10",
    "xmm11",
    "xmm12",
    "xmm13",
    "xmm14",
    "xmm15"
};

/*----------------------------------------------------------------------------*/

/* Generic x86_64 assembly instruction class.
 *
 * For simplicity and codebase compactness, it contains the maximum number of
 * operands any x64 ASM instruction could possibly need, regardless of how many
 * operands a particular instruction represented by this class actually needs,
 * instead of having separate classes for ASM instructions that take a different
 * number of operands and a base class for what's common among them.
 *
 * emit_asm_code() emits Intel-syntax assembly code of this instruction.
 *
 * Internally in the compiler, the generated assembly language program is held
 * within a cache locality-friendly contiguous memory arena and a directory to
 * navigate the arena, much like how the AST and the SSA IR program are
 * internally stored.
 */
class x64_Assembly_Instruction
{
public:
    constexpr static size_t x64_arch_max_insn_operand_arity = 4;
    const size_t insn_name_ix;
    const size_t insn_operand_arity;

    std::array<x64_ASM_Instruction_Operand,
               x64_arch_max_insn_operand_arity> insn_operands;

    /* Constructor. */
    explicit
    x64_Assembly_Instruction(size_t insn_name_ix_in, size_t operand_arity_in)
    : insn_name_ix(insn_name_ix_in), insn_operand_arity(operand_arity_in) {}

    /* TODO: Put this comment block in documentation file, not here in code.
     *
     * For a real file accessed through a FILE*, grab its file descriptor
     * using fileno(my_FILE_ptr) and pass that file descriptor to here.
     *
     * The reverse operation exists too. Since a FILE* lets you do useful
     * things like fgets(), fprintf(), etc, you can take an existing file
     * descriptor and wrap it in a FILE* using fdopen(int fd, const char *mode).
     *
     * Low-latency code tends to stay away from FILE* and work on fd's directly,
     * using read() and write().
     */
    void emit_asm_code(int output_fd) const
    {
        /* The instruction mnemonic. */
        write(output_fd, lookup_table_x64_insn_names[insn_name_ix],
              strlen(lookup_table_x64_insn_names[insn_name_ix]));

        /* A space. */
        write(output_fd, " ", 1);

        /* The instruction operands. */
        for(size_t i = 0; i < insn_operand_arity; ++i)
        {
            insn_operands[i].emit_asm_code();

            if(i != insn_operand_arity - 1)
                write(output_fd, ", ", 2);
        }

        /* Instruction assembly code has been emitted. Go on a new line. */
        write(output_fd, "\n", 1);
    }
};

/*----------------------------------------------------------------------------*/

/* Named x64 instruction operand kinds. */
constexpr size_t x64_operand_type_immediate                 = 0;
constexpr size_t x64_operand_type_reg                       = 1;
constexpr size_t x64_operand_type_reg_as_ptr                = 2;
constexpr size_t x64_operand_type_reg_plus_immediate_as_ptr = 3;
constexpr size_t x64_operand_type_complex_array_elem_access = 4;
constexpr size_t x64_operand_type_label                     = 5;

/* Generic x86_64 assembly instruction operand class.
 *
 * For simplicity and codebase compactness, it contains the maximum number of
 * parts that make up an x64 instruction operand, regardless of how many parts
 * a particular operand represented by this class actually has, instead of
 * having separate classes for operands that have a different number of parts to
 * them and a base class for what's common among them.
 *
 * emit_asm_code() emits Intel-syntax assembly code of this operand.
 */

class x64_ASM_Instruction_Operand
{
public:
    size_t operand_type;

    uint8_t operand_reg1_ix;
    uint8_t operand_reg2_ix;
    size_t  operand_immediate1_val;
    size_t  operand_immediate2_val;
    std::string_view operand_label;

    /* Constructor. */
    explicit x64_ASM_Instruction_Operand
        (size_t type_in, uint8_t reg1_ix_in, uint8_t reg2_ix_in,
         size_t immediate1_val_in, size_t immediate2_val_in, std::string str_in)
    : operand_type(type_in), operand_reg1_ix(reg1_ix_in),
      operand_reg2_ix(reg2_ix_in), operand_immediate1_val(immediate1_val_in),
      operand_immediate2_val(immediate2_val_in), operand_label(str_in) {}

    void emit_asm_code(int output_fd) const
    {
        constexpr size_t temp_string_buf_size = 32;
        uint8_t immediate_as_string[temp_string_buf_size];

        switch(operand_type)
        {
        case x64_operand_type_immediate:
        {
            memset(immediate_as_string, 0x00, temp_string_buf_size);
            sprintf(immediate_as_string, "%lu", operand_immediate1_val);
            write(output_fd, immediate_as_string, strlen(immediate_as_string));
            break;
        }
        case x64_operand_type_reg:
        {
            write(output_fd, lookup_table_x64_reg_names[operand_reg1_ix],
                  strlen(lookup_table_x64_reg_names[operand_reg1_ix]));
            break;
        }
        case x64_operand_type_reg_as_ptr:
        {
            write(output_fd, "[", 1);
            write(output_fd, lookup_table_x64_reg_names[operand_reg1_ix],
                  strlen(lookup_table_x64_reg_names[operand_reg1_ix]));
            write(output_fd, "]", 1);
            break;
        }
        case x64_operand_type_reg_plus_immediate_as_ptr:
        {
            write(output_fd, "[", 1);
            write(output_fd, lookup_table_x64_reg_names[operand_reg1_ix],
                  strlen(lookup_table_x64_reg_names[operand_reg1_ix]));
            write(output_fd, " + ", 3);
            sprintf(immediate_as_string, "%lu", operand_immediate1_val);
            write(output_fd, immediate_as_string, strlen(immediate_as_string));
            write(output_fd, "]", 1);
            break;
        }
        case x64_operand_type_complex_array_elem_access:
        {
            write(output_fd, "[", 1);
            write(output_fd, lookup_table_x64_reg_names[operand_reg1_ix],
                  strlen(lookup_table_x64_reg_names[operand_reg1_ix]));
            write(output_fd, " * ", 3);
            write(output_fd, lookup_table_x64_reg_names[operand_reg2_ix],
                  strlen(lookup_table_x64_reg_names[operand_reg2_ix]));
            write(output_fd, " + ", 3);
            sprintf(immediate_as_string, "%lu", operand_immediate1_val);
            write(output_fd, immediate_as_string, strlen(immediate_as_string));
            write(output_fd, " + ", 3);
            sprintf(immediate_as_string, "%lu", operand_immediate2_val);
            write(output_fd, immediate_as_string, strlen(immediate_as_string));
            write(output_fd, "]", 1);
            break;
        }
        case x64_operand_type_label:
        {
            write
               (output_fd, operand_label.c_str(), strlen(operand_labe.c_str()));
            break;
        }
        default:
        {
            break;
        } /* end last case. */
        } /* end switch.    */
    }
};

/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*     APPLICATION BINARY INTERFACE conventions for x86_64 on GNU/Linux.      */
/******************************************************************************/

/* How many of the first function arguments are passed in registers. */
constexpr size_t x64_int_function_args_to_regs_arity   = 6;
constexpr size_t x64_float_function_args_to_regs_arity = 8;

/* The exact registers that the first function arguments get passed in. */
constexpr std::array<size_t, x64_int_function_args_to_regs_arity>
x64_regs_to_pass_int_function_args_to =
{
    x64_reg_rdi_ix, x64_reg_rsi_ix, x64_reg_rdx_ix, x64_reg_rcx_ix,
    x64_reg_r8_ix, x64_reg_r9_ix
};

constexpr std::array<size_t, x64_float_function_args_to_regs_arity>
x64_regs_to_pass_float_function_args_to =
{
    x64_reg_xmm0_ix, x64_reg_xmm1_ix, x64_reg_xmm2_ix, x64_reg_xmm3_ix,
    x64_reg_xmm4_ix, x64_reg_xmm5_ix, x64_reg_xmm6_ix, x64_reg_xmm7_ix
};

/* Register for functions to return their value in - integer/floating point. */
constexpr size_t x64_function_int_ret_reg   = x64_reg_rax_ix;
constexpr size_t x64_function_float_ret_reg = x64_reg_xmm0_ix;

/* Register for functions that return composite structs. */
constexpr size_t x64_function_struct_ret_input_ptr_reg  = x64_reg_rdi_ix;
constexpr size_t x64_function_struct_ret_output_ptr_reg = x64_reg_rax_ix;

/* All callee-saves registers in the basic x64 machine description. */
constexpr size_t x64_callee_saves_regs_arity = 6;

constexpr std::array<size_t, x64_callee_saves_regs_arity>
x64_callee_saves_regs =
{
    x64_reg_rbx_ix, x64_reg_rbp_ix, x64_reg_r12_ix, x64_reg_r13_ix,
    x64_reg_r14_ix, x64_reg_r15_ix
};

/* All caller-saves registers in the basic x64 machine description. */

/* Minus 1 because RSP is neither caller-saved nor callee-saved. */
constexpr size_t x64_caller_saves_regs_arity =
         x64_basicMD_supported_asm_regs_arity - x64_callee_saves_regs_arity - 1;

constexpr std::array<size_t, x64_caller_saves_regs_arity>
x64_caller_saves_regs =
{
    x64_reg_rax_ix,   x64_reg_rcx_ix,   x64_reg_rdx_ix,
    x64_reg_r8_ix,    x64_reg_r9_ix,    x64_reg_r10_ix,   x64_reg_r11_ix,
    x64_reg_rdi_ix,   x64_reg_rsi_ix,
    x64_reg_xmm0_ix,  x64_reg_xmm1_ix,  x64_reg_xmm2_ix,  x64_reg_xmm3_ix,
    x64_reg_xmm4_ix,  x64_reg_xmm5_ix,  x64_reg_xmm6_ix,  x64_reg_xmm7_ix,
    x64_reg_xmm8_ix,  x64_reg_xmm9_ix,  x64_reg_xmm10_ix, x64_reg_xmm11_ix,
    x64_reg_xmm12_ix, x64_reg_xmm13_ix, x64_reg_xmm14_ix, x64_reg_xmm15_ix
};

/* TODO: When functions are in the language:
 *
 * Some status flags in RFLAGS register need to be caller-saved if the caller
 * is gonna be using them and wants them preserved across function calls, as
 * functions are freely able to edit and check those status flags.
 */

/* Stack pointer register RSP alignment requirement.
 *
 * The OS gives us a guarantee that RSP *will* be divisible by that number when
 * it spawns an OS process and our program starts running.
 *
 * We only have to make sure this is still the case right before a CALL
 * instruction is to be executed. Depending on what we have placed on the
 * caller's stack frame, RSP may not be aligned to 16 bytes at the CALL
 * instruction. If that's the case, emit "sub rsp, N" to properly align RSP.
 *
 * The return address is automatically pushed on the stack by the CALL
 * instruction, which means right AFTER the CALL instruction executes, RSP will
 * be divisible by 8.
 */
constexpr size_t x64_stack_pointer_reg_alignment_req_before_call = 16;

/* The machine word size.
 *
 * Pointers are always this size in bytes. Basic general purpose registers
 * are this size in bytes. size_t is defined to be this size in bytes.
 */
constexpr size_t x64_machine_word_size_bytes = 8;
