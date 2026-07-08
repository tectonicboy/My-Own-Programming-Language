/* Receives from a Parsing Orchestrator:
 *
 *  - Populated Auxilliary Code Block Statement Directory
 *  - Populated AST Arena
 *  - Populated Symbol Table
 *
 * Initializes:
 *
 *  - A new empty IR Instructions Arena
 *  - Total size in bytes of the IR Instructions Arena
 *  - Currently used bytes in the IR Instructions Arena = zero
 *  - Pointer to next free region of the IR Instructions Arena = start of it
 *
 * Gives jobs to one or more IR Generators by giving them quotas for which
 * Code Block's statement AST Nodes to emit IR instructions for and their own
 * regions of the IR Instructions Arena to emit their IR code to.
 */
class IR_Generation_Orchestrator {

public:
    /* Receives these from a Parsing_Orchestrator: */
    std::unordered_map<std::string, Symbol> Symbol_Table;
    uint8_t* ast_arena;
    std::vector<std::tuple<size_t, size_t, size_t>> statement_directory;
    const size_t statement_directory_used_entries;

    /* Brings into existence these new things: */
    uint8_t* IR_instructions_arena;
    size_t IR_instructions_arena_size;
    size_t IR_instructions_arena_free_region_offset;
    size_t IR_instructions_arena_used_bytes;

    /* Receives this from the main compilation driver: */
    std::vector<std::vector<size_t>> IR_generation_quotas;

    /* Constructor */
    explicit IR_Generation_Orchestrator
        (std::unordered_map<std::string,
         Symbol>&& sym_table_in,
         uint8_t* ast_arena_in,
         std::vector<std::tuple<size_t, size_t, size_t>>&& statement_dir_in,
         const size_t statement_dir_num_used_entries_in,
         std::vector<std::vector<size_t>>&& IR_gen_quotas_in
        )
    : Symbol_Table(std::move(sym_table_in)),
      /* TODO: Define the AST Arena and IR Instruction Arena and other memory
       *       arenas as classes (possibly even a generic (but not abstract)
       *       class Memory_Arena or Custom_Allocator that handles the arena)
       *       and defined a MOVE CONSTRUCTOR for it, so we can move the arenas
       *       between the various owning Lvalues (orchestrators) neatly.
       */
      ast_arena(ast_arena_in),
      statement_directory(std::move(statement_dir_in)),
      statement_directory_used_entries(statement_dir_num_used_entries_in),
      IR_instructions_arena_size(100'000),
      IR_instructions_arena_free_region_offset(0),
      IR_instructions_arena_used_bytes(0),
      /* TODO: The quotas vector of vectors ownership movement is incomplete.
       *       When the ParsingOrchestrator gets it, it doesnt take ownership
       *       from the main compilation driver. Make it so that it does, and
       *       then it transfers that ownership to here when AST is constructed.
       */
      IR_generation_quotas(std::move(IR_gen_quotas_in))
    {
        IR_instructions_arena = (uint8_t*)malloc(IR_instructions_arena_size);
        if(IR_instructions_arena == NULL)
        {
            std::cout << "Internal compiler error: "
                         "Allocating the IR instructions arena failed.\n";
            perror("errno: ");
            std::abort();
        }
        memset(IR_instructions_arena, 0x00, IR_instructions_arena_size);
    }
    uint8_t spawn_IR_generator(std::vector<size_t> IR_generation_quota);
};

/* Receives from an IR Generation Orchestrator:
 *
 *  - Quota: vector<size_t> of block indices whose AST statements to emit IR of.
 *  - Populated Auxilliary Code Block Statement Directory
 *  - Populated AST Arena
 *  - Populated Symbol Table
 *  - Byte offset to an empty region of the IR Instructions Arena
 *  - How much memory it's allowed to use in the IR Instructions Arena
 *  - Pointer to which it should write how much memory it ended up using.
 *
 * Produces:
 *
 *  - Populated IR Instructions Arena region.
 *  - Updated counter through passed pointer: Used IR instructions arena bytes.
 *
 * Its own auxilliary members:
 *
 *  - Counter: How many IR intermediates were emitted.
 *             An intermediate is emitted when the LHS and/or RHS of a
 *             binary operation is itself a binary operation.
 *
 *  - Counter: How many u64 literals have been placed in their own storage.
 *
 */
class IR_Generator {
    /* Receives from an IR Generation Orchestrator: */
    std::unordered_map<std::string, Symbol>* Symbol_Table;
    uint8_t* ast_arena;
    std::vector<std::tuple<size_t, size_t, size_t>>* statement_directory;
    const size_t statement_directory_used_entries;
    uint8_t* IR_instructions_arena;
    const size_t IR_instructions_arena_size;
    const size_t IR_instructions_arena_free_region_offset;

    /* This is a local counter for used bytes, by this IR Generator.
     * The IR Generation Orchestrator's file-global counter will be increased
     * via this pointer to this IR Generator's local counter, once it finishes
     * generating the IR instructions for its quota. This pointer gets passed
     * to the IR Generator by the IR Generation Orchestrator.
     */
    size_t* IR_instructions_arena_used_bytes;

    std::vector<size_t> IR_generation_quota;

    uint64_t IR_intermediates_emitted;
    uint64_t u64_literals_seen;
    uint64_t IR_instructions_emitted;

    /* Constructor */
    explicit IR_Generator
        (std::unordered_map<std::string, Symbol>* sym_table_ptr_in,
         uint8_t* ast_arena_ptr_in,
         std::vector<std::tuple<size_t, size_t, size_t>>* statement_dir_ptr_in,
         const size_t statement_dir_used_entries_in,
         uint8_t* IR_instructions_arena_ptr_in,
         const size_t IR_instructions_arena_size_in,
         const size_t IR_instructions_arena_free_region_offset_in,
         size_t* IR_instructions_arena_used_bytes_ptr_in,
         std::vector<size_t> IR_generation_quota_in
        )
    : Symbol_Table(sym_table_ptr_in),
      ast_arena(ast_arena_ptr_in),
      statement_directory(statement_dir_ptr_in),
      statement_directory_used_entries(statement_dir_used_entries_in),
      IR_instructions_arena(IR_instructions_arena_ptr_in),
      IR_instructions_arena_size(IR_instructions_arena_size_in),
      IR_instructions_arena_free_region_offset
        (IR_instructions_arena_free_region_offset_in),
      IR_instructions_arena_used_bytes(IR_instructions_arena_used_bytes_ptr_in),
      IR_generation_quota(IR_generation_quota_in),
      IR_intermediates_emitted(0),
      u64_literals_seen(0),
      IR_instructions_emitted(0),
    {}

    uint8_t emit_IR(void);

    uint8_t emit_IR_for_block(size_t ast_node_first_statement_index,
                              size_t block_index_in_statement_dir);

    uint8_t emit_IR_for_assignment(AST_Node_Statement_Assignment*);

    /* The IR instruction emitter functions will handle any padding bytes
     * because of alignment requirements themselves. Therefore, output pointer
     * passed to it should be at the immediate free byte where the last IR
     * instruction's object ends in the arena.
     */
    uint8_t emit_IR_insn_EQU(std::string lhs, std::string_rhs);

    uint8_t emit_IR_insn_ADD(std::string lhs_operand, std::string rhs_operand,
                             std::string target);

    uint8_t emit_IR_insn_SUB(std::string lhs_operand, std::string rhs_operand,
                             std::string target);

    uint8_t emit_IR_insn_MUL(std::string lhs_operand, std::string rhs_operand,
                             std::string target);

    uint8_t emit_IR_insn_DIV(std::string lhs_operand, std::string rhs_operand,
                             std::string quotient, std::string remainder);
};

/* The main compilation driver chooses which IR generation quota to process. */
uint8_t IR_Generation_Orchestrator::spawn_IR_generator
        (std::vector<size_t> selected_IR_generation_quota)
{
    uint8_t ret = 0;
    size_t this_generator_used_arena_bytes = 0;

    IR_Generator my_IR_generator(this->Symbol_Table,
                                 this->ast_arena,
                                 this->statement_directory,
                                 this->statement_directory_used_entries,
                                 this->IR_instructions_arena,
                                 this->IR_instructions_arena_size,
                                 this->IR_instructions_arena_free_region_offset,
                                 &this_generator_used_arena_bytes,
                                 selected_IR_generation_quota);

    ret = my_IR_generator.emit_IR();

    if(ret)
    [[unlikely]]
    {
        std::cout << "Not enough Arena memory for IR instructions of blocks:\n";
        for(size_t i = 0; i < selected_IR_generation_quota.size(); ++i)
        {
            std::cout << selected_IR_generation_quota[i];
        }
        std::cout << "\nGiving the IR Instructions Arena more memory.\n";
        return 1;
    }

    this->IR_instructions_arena_used_bytes += this_generator_used_arena_bytes;
    return 0;
}

uint8_t IR_Generator::emit_IR(void)
{
    uint8_t ret;
    bool end_of_ACBSD_reached = false;
    size_t curr_statement_dir_ix = 0;
    size_t curr_quota_block_ix;
    size_t curr_ast_arena_offset;
    AST_Node_Statement* cur_stmt_ast_node;

    /* The IR generation quota has indices of Code Blocks given in ascending
     * order. The Auxilliary Code Block Statement Directory has Code Block
     * indices along with each statement AST Node's offset into the AST Arena.
     *
     * So, go over each entry in the Quota and emit IR for that Code Block's
     * statements AST Nodes.
     */
    for(size_t i = 0; i < this->IR_generation_quota.size(); ++i)
    {
        /* Find this Code Block index's first Statement in the Auxilliary
         * Code Block Statement Directory. Go to its indicated offset into the
         * AST Arena. Emit IR instruction(s) for this Statement AST Node. Do
         * the same for all next Statement AST Nodes until the Code Block index
         * in the next ACBSD entry is not the one our quota said to work on.
         */
        curr_quota_block_ix = IR_generation_quota[i];

        while(std::get<STMT_DIR_BLOCK_INDEX>
                ((*statement_directory)[curr_statement_dir_ix])
              != curr_quota_block_ix)
        {
            ++curr_statement_dir_ix;
            if(curr_statement_dir_ix == this->statement_directory_used_entries)
            {
                std::cout << "IR_Generator::emit_IR : Reached end of ACBSD.\n";
                end_of_ACBSD_reached = true;
                break;
            }
        }
        if(end_of_ACBSD_reached == true) [[unlikely]] { break; }

        while(std::get<STMT_DIR_BLOCK_INDEX>
                ((*statement_directory)[curr_statement_dir_ix])
              == curr_quota_block_ix)
        {
            /* We've found the first ACBSD Statement to emit IR for. */
            curr_ast_arena_offset = std::get<STMT_DIR_NODE_ARENA_OFFSET>
                                ((*statement_directory)[curr_statement_dir_ix]);

            /* What Statement type is it? */
            cur_stmt_ast_node =
                (AST_Node_Statement*)(ast_arena + curr_ast_arena_offset);

            if
             (cur_stmt_ast_node->statement_kind_ix == STATEMENT_KIND_ASSIGNMENT)
            [[likely]]
            {
                ret = emit_IR_for_assignment
                            ((AST_Node_Statement_Assignment*)cur_stmt_ast_node);
                if(ret)
                {
                    std::cout << "[INFO] Not enough Arena memory to emit IR for"
                                 " an assignment statement node.\n";
                    return 1;
                }
            }
            else
            [[unlikely]]
            {
                std::cout << "Internal compiler error: IR_Generator::emit_IR: "
                             "Invalid statement type in AST Node.\n";
                std::abort();
            }

            /* Go to next ACBSD entry. Make sure we haven't reached its end. */
            ++curr_statement_dir_ix;

            if(curr_statement_dir_ix == this->statement_directory_used_entries)
            [[unlikely]]
            {
                std::cout << "IR_Generator::emit_IR : Reached end of ACBSD.\n";
                end_of_ACBSD_reached = true;
                break;
            }
        }
        if(end_of_ACBSD_reached == true) [[unlikely]] { break; }
    }

    std::cout << "[OK] IR Generator finished quota! IR Instructions emitted!\n";

    return 0;
}

uint8_t
IR_Generator::emit_IR_for_assignment(AST_Node_Statement_Assignment* stmt_node)
{

}
