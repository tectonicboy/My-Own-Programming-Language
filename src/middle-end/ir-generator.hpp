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
    size_t   IR_instructions_arena_size;
    uint8_t* IR_instructions_arena;
    size_t   IR_instructions_arena_free_region_offset;
    size_t   IR_instructions_arena_used_bytes;

    size_t IR_instructions_dir_init_size;
    size_t IR_instructions_dir_used_entries;
    std::vector<IR_Instructions_Directory_Entry> IR_instructions_directory;

    /* Receives this from the top-level compilation driver: */
    std::vector<std::vector<size_t>> IR_generation_quotas;

    /* Constructor */
    explicit IR_Generation_Orchestrator
        (std::unordered_map<std::string, Symbol>&& sym_table_in,
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
      IR_instructions_dir_init_size(10'000),
      IR_instructions_dir_used_entries(0),
      IR_instructions_directory(IR_instructions_dir_init_size,
            IR_Instructions_Directory_Entry(0, 0, 0, 0, 0)),
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

public:
    /* Receives from an IR Generation Orchestrator: */
    std::unordered_map<std::string, Symbol>* Symbol_Table;
    uint8_t* ast_arena;
    std::vector<std::tuple<size_t, size_t, size_t>>* statement_directory;
    const size_t statement_directory_used_entries;
    uint8_t* IR_instructions_arena;
    const size_t IR_instructions_arena_size;
    const size_t IR_instructions_arena_free_region_offset;
    const size_t IR_instructions_arena_bytes_available;

    /* This is a local counter for used bytes, by this IR Generator.
     * The IR Generation Orchestrator's file-global counter will be increased
     * via this pointer to this IR Generator's local counter, once it finishes
     * generating the IR instructions for its quota. This pointer gets passed
     * to the IR Generator by the IR Generation Orchestrator.
     */
    size_t* IR_instructions_arena_used_bytes;

    const size_t IR_instructions_dir_first_free_entry;
    const size_t IR_instructions_dir_available_entries;
    std::vector<IR_Instructions_Directory_Entry>* IR_instructions_directory;
    size_t* IR_instructions_dir_used_entries;

    std::vector<size_t> IR_generation_quota;

    size_t IR_intermediates_emitted;
    size_t count_u64_literals_seen;
    size_t IR_instructions_emitted;
    size_t count_statements_it_emitted_IR_for;

    const size_t encountered_u64_literals_array_init_size;
    size_t encountered_u64_literals_array_cur_size;
    std::vector<size_t> encountered_u64_literals_array;

    /* Constructor */
    explicit IR_Generator
        (std::unordered_map<std::string, Symbol>* sym_table_ptr_in,
         uint8_t* ast_arena_ptr_in,
         std::vector<std::tuple<size_t, size_t, size_t>>* statement_dir_ptr_in,
         const size_t statement_dir_used_entries_in,
         uint8_t* IR_instructions_arena_ptr_in,
         const size_t IR_instructions_arena_size_in,
         const size_t IR_instructions_arena_free_region_offset_in,
         const size_t IR_instructions_arena_bytes_available_in,
         size_t* IR_instructions_arena_used_bytes_ptr_in,
         const size_t IR_insns_dir_first_free_entry_in,
         const size_t IR_insns_dir_available_entries_in,
         std::vector<IR_Instructions_Directory_Entry>* IR_instructions_dir_in,
         size_t* IR_insn_dir_used_entries_in,
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
      IR_instructions_arena_bytes_available
        (IR_instructions_arena_bytes_available_in),
      IR_instructions_arena_used_bytes(IR_instructions_arena_used_bytes_ptr_in),
      IR_instructions_dir_first_free_entry(IR_insns_dir_first_free_entry_in),
      IR_instructions_dir_available_entries(IR_insns_dir_available_entries_in),
      IR_instructions_directory(IR_instructions_dir_in),
      IR_instructions_dir_used_entries(IR_insn_dir_used_entries_in),
      IR_generation_quota(IR_generation_quota_in),
      IR_intermediates_emitted(0),
      count_u64_literals_seen(0),
      IR_instructions_emitted(0),
      count_statements_it_emitted_IR_for(0),
      encountered_u64_literals_array_init_size(1'000),
      encountered_u64_literals_array_cur_size(0)
    {
        encountered_u64_literals_array.reserve
            (encountered_u64_literals_array_init_size);
    }

    uint8_t emit_IR(void);

    uint8_t emit_IR_for_block(size_t ast_node_first_statement_index,
                              size_t block_index_in_statement_dir);

    uint8_t emit_IR_for_assignment(AST_Node_Statement_Assignment* stmt_node,
                                     const size_t code_block_ix,
                                     const size_t statement_ix);

    /* The IR instruction emitter functions will handle any padding bytes
     * because of alignment requirements themselves. Therefore, output pointer
     * passed to it should be at the immediate free byte where the last IR
     * instruction's object ends in the arena.
     */
    uint8_t emit_IR_insn_EQU(std::string lhs, std::string rhs,
                             size_t* cur_wr_offset,
                             const size_t bytes_available,
                             const size_t code_block_ix,
                             const size_t statement_ix,
                             const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_ADD(std::string lhs_operand, std::string rhs_operand,
                             std::string target,
                             size_t* cur_wr_offset,
                             const size_t bytes_available,
                             const size_t code_block_ix,
                             const size_t statement_ix,
                             const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_SUB(std::string lhs_operand, std::string rhs_operand,
                             std::string target,
                             size_t* cur_wr_offset,
                             const size_t bytes_available,
                             const size_t code_block_ix,
                             const size_t statement_ix,
                             const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_MUL(std::string lhs_operand, std::string rhs_operand,
                             std::string target,
                             size_t* cur_wr_offset,
                             const size_t bytes_available,
                             const size_t code_block_ix,
                             const size_t statement_ix,
                             const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_DIV(std::string lhs_operand, std::string rhs_operand,
                             std::string quotient, std::string remainder,
                             size_t* cur_wr_offset,
                             const size_t bytes_available,
                             const size_t code_block_ix,
                             const size_t statement_ix,
                             const size_t ir_instruction_ix);
};

/* The main compilation driver chooses which IR generation quota to process. */
uint8_t IR_Generation_Orchestrator::spawn_IR_generator
        (std::vector<size_t> selected_IR_generation_quota)
{
    uint8_t ret = 0;
    size_t this_generator_used_IR_arena_bytes = 0;
    size_t this_generator_used_IR_dir_entries = 0;

    IR_Generator my_IR_generator(&(this->Symbol_Table),
                                 this->ast_arena,
                                 &(this->statement_directory),
                                 this->statement_directory_used_entries,
                                 this->IR_instructions_arena,
                                 this->IR_instructions_arena_size,
                                 this->IR_instructions_arena_free_region_offset,
                                 this->IR_instructions_arena_size,
                                 &this_generator_used_IR_arena_bytes,
                                 /* TODO: This won't be hardcoded to 0 when real
                                  *       multithreaded arena IR instructions
                                  *       generation gets implemented.
                                  */
                                 0,
                                 this->IR_instructions_directory.size(),
                                 &(this->IR_instructions_directory),
                                 &this_generator_used_IR_dir_entries,
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

    else
    {
        std::cout << "\n  ****  IR generation successful!  ****\n\n";
        std::cout << "IR Instructions emitted: "
                  << this_generator_used_IR_dir_entries << "\n";
        std::cout << "IR Arena bytes used: "
                  << this_generator_used_IR_arena_bytes << "\n";
    }

    this->IR_instructions_arena_used_bytes +=
        this_generator_used_IR_arena_bytes;
    this->IR_instructions_dir_used_entries +=
        this_generator_used_IR_dir_entries;

    return 0;
}

/* Top-level driving function for emitting IR for a given quota of Code Blocks.
 *
 * This function simply goes over the input IR generation quota, which is just
 * Code Block indices to emit IR for, finds the entries in Statement Directory
 * of these Code Blocks, looks at the memory offset for each Statement AST Node,
 * again recorded in each Statement Directory entry, goes into the AST Arena
 * where each Statement AST Node lives and uses it to emit initial IR code for
 * each source Statement by calling its respective IR emitting function.
 *
 * The different scenarios that change how IR code is emitted for a Statement,
 * depending on its structure and type, are handled by its respective IR
 * emitting function.
 */
uint8_t IR_Generator::emit_IR(void)
{
    uint8_t ret;
    bool end_of_ACBSD_reached = false;
    size_t curr_statement_dir_ix = 0;
    size_t curr_quota_block_ix;
    size_t curr_ast_arena_offset;
    size_t statements_we_emitted_IR_for = 0;
    AST_Node_Statement* cur_stmt_ast_node;

    /* Go over the quota entries and emit IR code for the given Code Blocks. */
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
            [[unlikely]]
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
            /* Grab statement's AST Arena offset from Statement Directory. */
            curr_ast_arena_offset = std::get<STMT_DIR_NODE_ARENA_OFFSET>
                                ((*statement_directory)[curr_statement_dir_ix]);

            /* Get stmt type and call its respective IR emitting function. */
            cur_stmt_ast_node =
                (AST_Node_Statement*)(ast_arena + curr_ast_arena_offset);

            if
             (cur_stmt_ast_node->statement_kind_ix == STATEMENT_KIND_ASSIGNMENT)
            [[likely]]
            {
                ret = emit_IR_for_assignment
                           ((AST_Node_Statement_Assignment*)cur_stmt_ast_node,
                             curr_quota_block_ix, statements_we_emitted_IR_for);
                if(ret == 1)
                [[unlikely]]
                {
                    std::cout << "[INFO] Not enough Arena memory to emit IR for"
                                 " an assignment statement node.\n";
                    return 1;
                }
                else if(ret == 2)
                [[unlikely]]
                {
                    std:: cout << "[INFO] Not enough entries in IR Instructions"
                                  " Directory.\n";
                    return 2;
                }
            }
            else
            [[unlikely]]
            {
                std::cout << "Internal compiler error: IR_Generator::emit_IR: "
                             "Invalid statement type in AST Node.\n";
                std::abort();

            }
            ++statements_we_emitted_IR_for;

            /* Go to next Statement Directory entry.
             * Make sure we haven't reached its end. */
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

    if(end_of_ACBSD_reached == true)
        std::cout << "IR Generator: End of Statement Directory was reached.\n";

    std::cout << "[OK] IR Generator finished its work quota.\n"
                 "     Source statements IR code was emitted for: "
              << statements_we_emitted_IR_for << "\n";

    return 0;
}

/* This function emits a single IR instruction for a single Assignment Statement
 * via its AST Node. Assignments from binary operations and from literals can
 * cause more IR instructions to be emitted beforehand, as each literal and
 * binary operation result get assigned to their own auxilliary IR variable.
 *
 * Three cases exist that have to be handled differently:
 *
 *  - Direct assignment from a literal: a = 5
 *
 *      1. Check whether this literal has an auxilliary IR variable already
 *         created for it by going over the vector<uint64_t> or the vector for
 *         other types of literals when we have them later in the language.
 *
 *      2. If found, use that literal's IR variable in the emitted instruction.
 *         Else, create a new auxilliary IR variable for the literal and use it.
 *
 *  - Direct assignment from another source variable: a = b
 *
 *      1. Use the RHS source variable's current counter in Symbol MINUS ONE,
 *         as the RHS IR variable in the emitted IR instruction, like %b_1.
 *         MINUS ONE because the current counter is the next version of that
 *         source variable next time it is assigned to. No need to check for use
 *         before initialization, the AST generator checked that already.
 *
 *  - Assignment from a binary operation: a = (b + 5), a = ( (b * b) + 5), etc.
 *
 *      1. Recursively check whether the Expression in LHS and RHS of this
 *         binary operation is itself a binary operation. Keep going until
 *         it's not. Abstract base class for Expression has a data member
 *         holding the Expression Type, which determines what derived Expression
 *         class object we are looking at. Then, create an intermediate IR
 *         variable holding the result of each simple binary operation.
 *
 *         Basically, each binary operation's result, starting from the deepest
 *         nested one (which by definition only has a Symbol as RHS and LHS),
 *         is stored in its own auxilliary IR variable, in order to maintain
 *         Static Single-Assignment Form. The newly created auxilliary IR
 *         variables are then used in the binary operations whose RHS/LHS was
 *         THAT nested binary operation, which the IR vaiable was created for.
 *  --------------------------------------------------------------------------
 *      LAST STEP: Determine the LHS variable of the emitted IR instruction:
 *
 *         Check this LHS source variable's counter in its Symbol. Use current
 *         counter to produce the new LHS IR variable, eg. %a_2.
 *         Increment that source variable's counter in its Symbol object.*
 */
uint8_t
IR_Generator::emit_IR_for_assignment(AST_Node_Statement_Assignment* stmt_node,
                                     const size_t code_block_ix,
                                     const size_t statement_ix)
{
    uint8_t assignment_rhs_expr_kind = stmt_node->rhs_expression->expr_kind_ix;
    std::string assignment_lhs_src_var = stmt_node->lhs_identifier->symbol_name;
    std::string assignment_rhs_var1;
    std::string assignment_rhs_var2;
    uint8_t ret = 0;
    size_t  i;
    bool    literal_has_already_been_encountered = false;
    size_t  u64_literals_encountered_arr_cur_siz;
    size_t  wr_offset = *(this->IR_instructions_arena_used_bytes);
    size_t  insns_emitted_for_this_stmt = 0;
    const size_t bytes_available = this->IR_instructions_arena_bytes_available;

    std::string ir_insn_target;
    std::string ir_insn_operand1;
    std::string ir_insn_operand2;

    /* RHS of each assignment is an expression of one of these kinds: */
    AST_Node_Expr_BinOp*          rhs_expr_binop       = nullptr;
    AST_Node_Expr_UINT64_Literal* rhs_expr_u64_literal = nullptr;
    AST_Node_Expr_Identifier*     rhs_expr_identifier  = nullptr;

    /* Check which of the 3 cases we're dealing with by looking at the RHS. */

    /* Case 1. Direct assignment from a literal: a = 5 */
    if(assignment_rhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
    {
        rhs_expr_u64_literal =
            (AST_Node_Expr_UINT64_Literal*)(stmt_node->rhs_expression);

        /* Look for the u64 literal in the array of already encountered ones. */
        u64_literals_encountered_arr_cur_siz =
            this->encountered_u64_literals_array.size();

        for(i = 0; i < u64_literals_encountered_arr_cur_siz; ++i)
        {
            if(rhs_expr_u64_literal->value
                == this->encountered_u64_literals_array[i])
            {
                literal_has_already_been_encountered = true;
                ir_insn_operand1 = "%const_" + std::to_string(i);
                break;
            }
        }
        if(literal_has_already_been_encountered == false)
        {
            /* Emit an additional IR instruction creating an auxilliary
             * IR variable to hold this literal, since we don't have one yet.
             * Record the fact that we've seen this literal in the source by
             * placing it in next entry of encountered_u64_literals_array.
             *
             * TODO: Isn't it weird that this array of encountered u64 literals
             *       (and every other type of literal for that matter, later on)
             *       is a data member of each spawned IR Generator object?
             *       The IR variable storing a literal can be INDEPENDENTLY used
             *       in all Code Blocks, so it should be part of the
             *       IR Generation Orchestrator, a pointer to it ought to be
             *       passed to each spawned IR Generator.
             *
             *       When multithreaded IR generation is added, so multiple
             *       threads, they can ATOMICALLY increment the INDEX for the
             *       next available entry. WARNING though: emplace, emplace_back
             *       are NOT THREAD-SAFE, the vector's auto-resizing mechanism
             *       is not thread-safe, so i have to manually use vec[ix] to
             *       add the entry by each thread, and this will be added ONLY
             *       AFTER each thread makes sure it's not writing past the
             *       currently reserved max number of elements.
             *
             *       The following can be used for this:
             *
             *       --- VISIBLE TO ALL THREADS: ---
             *       std::atomic<size_t> my_atomic_ix = 0;
             *
             *       --- IN THE THREAD FUNCTION: ---
             *       my_atomic_ix.fetch_add(num, SELECTED_MEMORY_ORDER);
             *       vec[my_atomic_ix] = ADD_ENTRY();
             *
             *       This won't matter for having only singlethreaded IR
             *       generation, but will when multithreaded IR generation is
             *       added. Several things will need updating then, similar to
             *       multithreaded AST generation, both are partially ready.
             */
            ir_insn_operand1 = std::string("%const_") +
                        std::to_string(u64_literals_encountered_arr_cur_siz);

            ret = emit_IR_insn_EQU (ir_insn_operand1,
                                    std::to_string(rhs_expr_u64_literal->value),
                                    &wr_offset, bytes_available, code_block_ix,
                                    statement_ix, insns_emitted_for_this_stmt);

            /* TODO: Check that ALL functions that could run out of space in
             *       an Arena and in a Directory or any other vairable-sized
             *       data structure don't just return 1 in all error cases.
             */
            if(ret) [[unlikely]] { return ret; }

            /* Place the newly recorded literal in the IR bookkeeping array. */
            this->encountered_u64_literals_array.emplace_back
                (rhs_expr_u64_literal->value);

            ++insns_emitted_for_this_stmt;
        }

        ir_insn_target =
               std::string("%") + assignment_lhs_src_var + std::string("_")
             + std::to_string(stmt_node->lhs_identifier->SSA_IR_mangle_counter);

        ret = emit_IR_insn_EQU
                (ir_insn_target,
                ir_insn_operand1,
                &wr_offset, bytes_available, code_block_ix, statement_ix,
                insns_emitted_for_this_stmt);

        if(ret) [[unlikely]] { return ret; }

        ++insns_emitted_for_this_stmt;
        stmt_node->lhs_identifier->SSA_IR_mangle_counter += 1;
    }

    /* Case 2. Direct assignment from another source variable: a = b */
    else if(assignment_rhs_expr_kind == EXPR_KIND_IDENTIFIER)
    {
        rhs_expr_identifier =
            (AST_Node_Expr_Identifier*)(stmt_node->rhs_expression);

        assignment_rhs_var1 = rhs_expr_identifier->symbol->symbol_name;

        ir_insn_operand1 =   std::string("%") + assignment_rhs_var1
                           + std::string("_") + std::to_string
                       (rhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1);

        ir_insn_target =   std::string("%") + assignment_lhs_src_var
                         + std::string("_") + std::to_string
                             (stmt_node->lhs_identifier->SSA_IR_mangle_counter);

        ret = emit_IR_insn_EQU
                (ir_insn_target,
                ir_insn_operand1,
                &wr_offset, bytes_available, code_block_ix, statement_ix,
                insns_emitted_for_this_stmt);

        if(ret) [[unlikely]] { return ret; }

        ++insns_emitted_for_this_stmt;
        stmt_node->lhs_identifier->SSA_IR_mangle_counter += 1;
    }

    /* Case 3. Assignment from a (possibly multi-level) binary operation. */
    else if(assignment_rhs_expr_kind == EXPR_KIND_BIN_OPERATION)
    {
        /* PLACEHOLDER, REMOVE THE ABORT WHEN I GET TO IMPLEMENTING THIS. */
        std::abort();
    }

    ++(this->count_statements_it_emitted_IR_for);
    ++(this->IR_instructions_emitted);
    *(this->IR_instructions_arena_used_bytes) = wr_offset;
    return 0;
}

uint8_t IR_Generator::emit_IR_insn_EQU(std::string lhs, std::string rhs,
                                       size_t* cur_wr_offset,
                                       const size_t bytes_available,
                                       const size_t code_block_ix,
                                       const size_t statement_ix,
                                       const size_t ir_instruction_ix)
{
    /* Save byte offset into Arena locally from passed pointer. */
    size_t wr_offset = *cur_wr_offset;

    /* Align if needed and make sure the Arena has enough memory. */
    while
      ( ((uintptr_t)(IR_instructions_arena + wr_offset))
       % alignof(ir_insn_equate) )
    { ++wr_offset; };

    if(wr_offset + sizeof(ir_insn_equate) > bytes_available)
    [[unlikely]]
    { return 1; }

    /* Make sure the IR Instructions Directory has available entries. */
    if(*(this->IR_instructions_dir_used_entries)
         == this->IR_instructions_dir_available_entries)
    [[unlikely]]
    { return 2; }

    /* Construct new IR instruction in IR Arena, advance byte offset tracker. */
    new (IR_instructions_arena + wr_offset) ir_insn_equate(lhs, rhs);
    wr_offset += sizeof(ir_insn_equate);

    /* Put entry in IR Instructions Directory, advance used_entries counter. */
    (*(this->IR_instructions_directory))
        [*(this->IR_instructions_dir_used_entries)]
        = IR_Instructions_Directory_Entry(code_block_ix, statement_ix,
                                          ir_instruction_ix,
                                          wr_offset - sizeof(ir_insn_equate),
                                          IR_INSN_EQUATE);

    ++(*(this->IR_instructions_dir_used_entries));

    /* Update passed pointer to Arena byte offset. */
    *cur_wr_offset = wr_offset;
    return 0;
}
