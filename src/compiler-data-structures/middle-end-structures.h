/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The IR Instructions Directory. */

/* Lookup table with named indices of the available SSA IR Instruction types. */

constexpr size_t TOTAL_IR_INSN_TYPES = 5;

constexpr size_t IR_INSN_EQUATE = 1;
constexpr size_t IR_INSN_ADD    = 2;
constexpr size_t IR_INSN_SUB    = 3;
constexpr size_t IR_INSN_MUL    = 4;
constexpr size_t IR_INSN_DIV    = 5;

constexpr std::array<const char*, TOTAL_IR_INSN_TYPES>
lookuptable_ir_insn_types =
{
    "IR Instruction EQUATE",
    "IR Instruction ADD",
    "IR Instruction SUB",
    "IR Instruction MUL",
    "IR Instruction DIV"
};

/* Descriptor for a single entry in the IR Instructions Directory.
 *
 * Each Code Block has one or more source statements. Each source statement
 * gets one or more emitted IR instructions that implement it in SSA IR code.
 */
class IR_Instructions_Directory_Entry
{
public:
    size_t code_block_ix;
    size_t statement_ix;
    size_t ir_insn_ix;
    size_t ir_insn_arena_offset;
    size_t which_ir_instruction;

    /* Constructor */
    explicit IR_Instructions_Directory_Entry
        (size_t code_block_ix_in, size_t statement_ix_in,
         size_t ir_insn_ix_in,    size_t ir_insn_arena_offset_in,
         size_t which_ir_insn_in)
    : code_block_ix(code_block_ix_in), statement_ix(statement_ix_in),
      ir_insn_ix(ir_insn_ix_in), ir_insn_arena_offset(ir_insn_arena_offset_in),
      which_ir_instruction(which_ir_insn_in) {}

    void print_entry(void)
    {
        std::cout << "Code Block          : " << code_block_ix        << "\n"
                  << "Statement           : " << statement_ix         << "\n"
                  << "IR Instruction Index: " << ir_insn_ix           << "\n"
                  << "IR Instruction Type : "
                  << lookuptable_ir_insn_types[which_ir_instruction]  << "\n"
                  << "IR Arena Offset     : " << ir_insn_arena_offset << "\n";
        return;
    }
};

class IR_Instructions_Directory
{
private:
    constexpr static size_t IR_instructions_dir_default_init_capacity = 10'000;
    size_t initial_capacity;
    std::vector<IR_Instructions_Directory_Entry> IR_instructions_dir_vec;

public:
    /* Regular constructor.
     *
     * Pass 0 for initial_capacity_in to use default initial capacity.
     *
     * When initializing the std::vector container itself, the caller either
     * tells us to only reserve memory capacity, or to go ahead and preconstruct
     * the entries, so the user can start writing to arbitrary indices.
     * Right now, the IR Generation Orchestrator only reserves memory capacity,
     * with .size() still starting out as 0.
     */
    explicit
    IR_Instructions_Directory(size_t init_capa_in, bool prefill_default_slots)
    {
        if( ! init_capa_in )
            initial_capacity = IR_instructions_dir_default_init_capacity;
        else
            initial_capacity = init_capa_in;

        if(prefill_default_slots)
            IR_instructions_dir_vec = std::vector
                (initial_capacity, IR_Instructions_Directory_Entry(0,0,0,0,0));
        else
            IR_instructions_dir_vec.reserve(initial_capacity);
    }

    /* Move constructor.
     *
     * Used when passing ownership of the IR Instructions Directory from the
     * Parsing Orchestrator to the IR Generation Orchestrator.
     */
    IR_Instructions_Directory(IR_Instructions_Directory&& old_dir)
    : initial_capacity(old_dir.initial_capacity),
      IR_instructions_dir_vec(std::move(old_dir.IR_instructions_dir_vec)) {}

    /* Overloaded operator[]. Both for getting a const and a mutable entry. */

    /* Returns a mutable Lvalue reference to an entry, directly modifyable. */
    IR_Instructions_Directory_Entry& operator[](const size_t entry_ix)
    {
        return IR_instructions_dir_vec[entry_ix];
    }
    /* Returns a const Lvalue reference to an entry, not modifyable. */
    const
    IR_Instructions_Directory_Entry& operator[](const size_t entry_ix) const
    {
        return IR_instructions_dir_vec[entry_ix];
    }

    /* Emplace an entry at the back. */
    inline void
    emplace_back
        (const size_t code_block_ix_in, const size_t stmt_ix_in,
         const size_t ir_insn_ix_in,    const size_t ir_insn_arena_offset_in,
         const size_t which_ir_insn_in)
    {
        IR_instructions_dir_vec.emplace_back
            (code_block_ix_in, stmt_ix_in, ir_insn_ix_in,
             ir_insn_arena_offset_in, which_ir_insn_in);
        return;
    }

    inline size_t size() const
    {
        return IR_instructions_dir_vec.size();
    }
};
