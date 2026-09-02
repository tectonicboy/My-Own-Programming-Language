/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Assembly Instructions Directory. */

/* Descriptor for a single entry in the Assembly Instructions Directory.
 *
 * Each Code Block has one or more source code statements. Each source statement
 * gets one or more SSA IR code instructions emitted for it. Each IR instruction
 * gets one or more assembly language instructions emitted for it. This class
 * describes one ASM instruction.
 */

class ASM_Instructions_Directory_Entry
{
public:
    size_t code_block_ix;
    size_t statement_ix;
    size_t ir_insn_ix;
    size_t asm_insn_ix;
    size_t asm_insn_arena_offset;

    /* Constructor. */
    explicit ASM_Instructions_Directory_Entry
        (size_t code_block_ix_in, size_t statement_ix_in, size_t ir_insn_ix_in,
         size_t asm_insn_ix_in, size_t asm_insn_arena_offset_in)
    : code_block_ix(code_block_ix_in), statement_ix(statement_ix_in),
      ir_insn_ix(ir_insn_ix_in), asm_insn_ix(asm_insn_ix_in),
      asm_insn_arena_offset(asm_insn_arena_offset_in) {}

    void print_entry(void) const
    {
        std::cout << "Code Block      Index: " << code_block_ix         << "\n"
                  << "Statement       Index: " << statement_ix          << "\n"
                  << "IR  Instruction Index: " << ir_insn_ix            << "\n"
                  << "ASM Instruction Index: " << asm_insn_ix           << "\n"
                  << "ASM Arena Offset     : " << asm_insn_arena_offset << "\n";
        return;
    }
};

class ASM_Instructions_Directory
{
private:
    constexpr static size_t ASM_instructions_dir_default_init_capacity = 10'000;
    size_t initial_capacity;
    std::vector<ASM_Instructions_Directory_Entry> ASM_instructions_dir_vec;

public:
    /* Regular constructor.
     *
     * Pass 0 for initial_capacity_in to use default initial capacity.
     *
     * When initializing the std::vector container itself, the caller either
     * tells us to only reserve memory capacity, or to go ahead and preconstruct
     * the entries, so the user can start writing to arbitrary indicies.
     * Right now, the ASM Generation Orchestrator only reserves memory capacity,
     * with .size() still starting out as 0.
     */
    explicit
    ASM_Instructions_Directory(size_t init_capa_in, bool prefill_default_slots)
    {
       if( ! init_capa_in )
            initial_capacity = ASM_instructions_dir_default_init_capacity;
        else
            initial_capacity = init_capa_in;

        if(prefill_default_slots)
            ASM_instructions_dir_vec = std::vector
                (initial_capacity, ASM_Instructions_Directory_Entry(0,0,0,0,0));
        else
            ASM_instructions_dir_vec.reserve(initial_capacity);
    }

    /* Move constructor. For completeness, unused for now. */
    ASM_Instructions_Directory(ASM_Instructions_Directory&& old_dir)
    : initial_capacity(old_dir.initial_capacity),
      ASM_instructions_dir_vec(std::move(old_dir.ASM_instructions_dir_vec)) {}

    /* Overloaded operator[]. Both for getting a const and a mutable entry. */

    /* Returns a mutable Lvalue reference to an entry, directly modifyable. */
    ASM_Instructions_Directory_Entry& operator[](const size_t entry_ix)
    {
        return ASM_instructions_dir_vec[entry_ix];
    }
    /* Returns a const Lvalue reference to an entry, not modifyable. */
    const
    ASM_Instructions_Directory_Entry& operator[](const size_t entry_ix) const
    {
        return ASM_instructions_dir_vec[entry_ix];
    }

    /* Emplace an entry at the back. */
    inline void
    emplace_back
        (const size_t code_block_ix_in, const size_t stmt_ix_in,
         const size_t ir_insn_ix_in, const size_t asm_insn_ix_in,
         const size_t asm_insn_arena_offset_in)
    {
        ASM_instructions_dir_vec.emplace_back
            (code_block_ix_in, stmt_ix_in, ir_insn_ix_in,
             asm_insn_ix_in, asm_insn_arena_offset_in);
        return;
    }

    inline size_t size() const
    {
        return ASM_instructions_dir_vec.size();
    }
};
