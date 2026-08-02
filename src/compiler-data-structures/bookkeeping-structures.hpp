/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Code Block Directory. */

/* Lookup table of Code Block types. */
constexpr size_t   total_code_block_types  = 1;
constexpr uint32_t CODE_BLOCK_TYPE_PROGRAM = 0;

constexpr std::array<const char*, total_code_block_types>
code_block_type_strings =
{
    "primary program code block"
};

/* Descriptor of a single entry in the Code Block Directory. */
class Code_Block_Directory_Entry
{
public:
    size_t start_token_index;
    size_t end_token_index;
    size_t code_block_type_index;

    /* Constructor. */
    Code_Block_Directory_Entry(size_t start_ix, size_t end_ix, size_t type_ix)
    : start_token_index(start_ix), end_token_index(end_ix),
      code_block_type_index(type_ix)
    {
        /* Handle error case: an invalid type index was somehow passed. */
        if(type_ix >= total_code_block_types)
        [[unlikely]]
        {
            std::cout << "\nCRITICAL: Internal compiler error. [LEXER]\n"
                      << "Code Block ctor: Passed type ix: " << type_ix << "\n"
                      << "Available type indices: 0 to "
                      << total_code_block_types - 1 << "\n" << "Aborting.\n\n";
            std::abort();
        }
    }
    void print_code_block_info(void)
    {
        std::cout << "\n--------------------------------------------------"
                  << "\nCode Block Type: "
                  << code_block_type_strings[code_block_type_index]
                  << "\nStart Token Index: " << start_token_index
                  << "\nEnd   Token Index: " << end_token_index
                  << "\n--------------------------------------------------\n";
    }
};

/* The Code Block Directory.
 *
 * Used exclusively in the compiler frontend. Spawned into existence by a Lexer,
 * then given to a Parser using move semantics via the custom move constructor.
 */
class Code_Block_Directory
{
private:
    constexpr static size_t code_block_dir_default_initial_capacity = 100;
    size_t initial_capacity;
    std::vector<Code_Block_Directory_Entry> code_block_dir_vec;
public:
    /* Regular constructor.
     *
     * Pass 0 for initial_capacity_in to use default initial capacity.
     *
     * When initializing the std::vector container itself, the caller either
     * tells us to only reserve memory capacity, or to go ahead and preconstruct
     * the entries, so the user can start writing to arbitrary indices.
     *
     * For now, the compiler only reserves capacity, size is still 0 at start
     * and entries are added via vector.emplace_back().
     */
    explicit
    Code_Block_Directory(size_t init_capa_in, bool prefill_default_slots)
    {
        if( ! init_capa_in )
            initial_capacity = code_block_dir_default_initial_capacity;
        else
            initial_capacity = init_capa_in;

        if(prefill_default_slots)
            code_block_dir_vec = std::vector
                        (initial_capacity, Code_Block_Directory_Entry(0, 0, 0));
        else
            code_block_dir_vec.reserve(initial_capacity);
    }

    /* Move constructor.
     *
     * Used when passing ownership of the Code Block Directory from the Lexer
     * to the Parsing Orchestrator, avoiding unnecessary copying.
     */
    Code_Block_Directory(Code_Block_Directory&& old_dir)
    : initial_capacity(old_dir.initial_capacity),
      code_block_dir_vec(std::move(old_dir.code_block_dir_vec))
    {}

    /* Overloaded operator[]. Both for getting a const and mutable a entry. */

    /* Returns a mutable Lvalue reference to an entry, directly modifyable. */
    Code_Block_Directory_Entry& operator[](const size_t entry_ix)
    {
        return code_block_dir_vec[entry_ix];
    }
    /* Returns a const Lvalue reference to an entry, not modifyable. */
    const Code_Block_Directory_Entry& operator[](const size_t entry_ix) const
    {
        return code_block_dir_vec[entry_ix];
    }
    /* Emplace an entry at the back. */
    void emplace_back(const size_t start_token_ix,
                      const size_t end_token_ix,
                      const size_t code_block_type_ix)
    {
        code_block_dir_vec.emplace_back(Code_Block_Directory_Entry
            (start_token_ix, end_token_ix, code_block_type_ix));
        return;
    }
    size_t size(void) const
    {
        return code_block_dir_vec.size();
    }
};


/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Statement Directory. */

/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The IR Instructions Directory. */

/* Descriptor for a single entry in the IR Instructions Directory.
 *
 * Each Code Block has one or more source statements. Each source statement
 * gets one or more emitted IR instructions that implement it in SSA IR code.
 */
class IR_Instructions_Directory_Entry {
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
      which_ir_instruction(which_ir_insn_in)
    {}

    void print_entry(void)
    {
        std::cout << "Code Block          : " << code_block_ix        << "\n"
                  << "Statement           : " << statement_ix         << "\n"
                  << "IR Instruction Index: " << ir_insn_ix           << "\n"
                  << "IR Instruction Type : " << which_ir_instruction << "\n"
                  << "IR Arena Offset     : " << ir_insn_arena_offset << "\n";
        return;
    }

};
