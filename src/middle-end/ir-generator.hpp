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
    size_t   IR_instructions_arena_size;
    size_t   IR_instructions_arena_free_region_offset;
    size_t   IR_instructions_arena_used_bytes;

    /* Receives this from the top-level compilation driver: */
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
    const size_t IR_instructions_arena_bytes_available;

    /* This is a local counter for used bytes, by this IR Generator.
     * The IR Generation Orchestrator's file-global counter will be increased
     * via this pointer to this IR Generator's local counter, once it finishes
     * generating the IR instructions for its quota. This pointer gets passed
     * to the IR Generator by the IR Generation Orchestrator.
     */
    size_t* IR_instructions_arena_used_bytes;

    std::vector<size_t> IR_generation_quota;

    size_t IR_intermediates_emitted;
    size_t count_u64_literals_seen;
    size_t IR_instructions_emitted;
    size_t count_statements_it_emitted_IR_for;

    const encountered_u64_literals_array_init_size;
    encountered_u64_literals_array_cur_size;
    std::vector<size_t> encountered_u64_literals;

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
      IR_generation_quota(IR_generation_quota_in),
      IR_intermediates_emitted(0),
      count_u64_literals_seen(0),
      IR_instructions_emitted(0),
      count_statements_it_emitted_IR_for(0),
      encountered_u64_literals_array_init_size(1000),
      encountered_u64_literals_array_cur_size(0),
    {
        encountered_u64_literals_array.reserve
            (encountered_u64_literals_array_init_size);
    }

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
                                 this->IR_instructions_arena_size,
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
 *         Increment that source variable's counter in its Symbol object.
 *
 *    Data members to keep track of, use and update when needed:
 *
 *    std::unordered_map<std::string, Symbol>* Symbol_Table;
 *    uint8_t* ast_arena;
 *    std::vector<std::tuple<size_t, size_t, size_t>>* statement_directory;
 *    const size_t statement_directory_used_entries;
 *
 *    uint8_t*     IR_instructions_arena;
 *    const size_t IR_instructions_arena_size;
 *    const size_t IR_instructions_arena_free_region_offset;
 *
 *    size_t* IR_instructions_arena_used_bytes;
 *
 *    std::vector<size_t> IR_generation_quota;
 *
 *    size_t IR_intermediates_emitted;
 *    size_t u64_literals_seen;
 *    size_t IR_instructions_emitted;
 *    size_t count_statements_it_emitted_IR_for;
 *
 *    const encountered_u64_literals_array_init_size;
 *    std::vector<size_t> encountered_u64_literals_array;
 */
uint8_t
IR_Generator::emit_IR_for_assignment(AST_Node_Statement_Assignment* stmt_node)
{
    uint8_t assignment_rhs_expr_kind = stmt_node->rhs_expression->expr_kind_ix;
    uint8_t ret = 0;
    size_t  i;
    bool    literal_has_already_been_encountered = false;
    size_t  u64_literals_encountered_arr_cur_siz;
    size_t  wr_offset = *(this->IR_instructions_arena_used_bytes);
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
            if(rhs_expr_u64_literal->value == this->encountered_u64_literals[i])
            {
                literal_has_already_been_encountered = true;
                ir_insn_operand1 = "%%const_" + std::to_string(i);
                break;
            }
        }
        if(literal_has_already_been_encountered == false)
        {
            /* Emit an extra IR instruction that creates the auxilliary IR
             * variable for this newly seen literal. */

            /* Align if needed. */
            while
             ( ((uintptr_t)(IR_instructions_arena + wr_offset))
               % alignof(ir_insn_equate) )
            { ++wr_offset };

            if(wr_offset + sizeof(ir_insn_equate) > bytes_available)
            [[unlikely]]
            { return 1; }

            /* Ready to construct the object for the extra IR instruction. */

            /* TODO: So, we can't just have IR instructions sitting around in
             *       an Arena. Because how will we be able to traverse the arena
             *       and find the IR instructions later? We won't know where
             *       they even start because of potential empty bytes for
             *       alignment requirements. We need a bookkeeping data
             *       structure similar to the Statement Directory, except this
             *       one will be the IR Instructions Directory. Each entry
             *       ought to contain these things:
             *
             *       - Index of the Code Block this IR instruction belongs to;
             *       - Index of the IR instruction within that Code Block;
             *       - Offset into the IR Instructions Arena of the object;
             *
             *  Once I add this IR Instructions Directory to the IR generation
             *  orchestrator and how it passes a pointer to it to the IR
             *  generators that it spawn, I can finish writing the code below.
             *
             *  I still won't be holding a pointer to the newly constructed
             *  object anywhere. IDK how to construct an object at a specified
             *  memory location without using NEW, but NEW always returns a
             *  pointer to the constructed object, which I don't need in this
             *  case. So look into ways of doing this without using NEW.
             *
             */
            if(new (IR_instructions_arena + wr_offset)
                ir_insn_equate(std::string("%%const_") + std::to_string(u64_literals_encountered_arr_cur_siz)))

            /* Finalize the first operand string of the main IR instruction. */
            ir_insn_operand1 = std::string("%%const_") +
                           std::to_string(u64_literals_encountered_arr_cur_siz);

            this->encountered_u64_literals_array.emplace_back
                (rhs_expr_u64_literal->value);
        }


    }

    /* Case 2. Direct assignment from another source variable: a = b */
    else if(assignment_rhs_expr_kind == EXPR_KIND_IDENTIFIER)
    {

    }

    /* Case 3. Assignment from a (possibly multi-level) binary operation. */
    else if(assignment_rhs_expr_kind == EXPR_KIND_BIN_OPERATION)
    {

    }

    ++(this->count_statements_it_emitted_IR_for);
    ++(this->IR_instructions_emitted);
    *(this->IR_instructions_arena_used_bytes) += wr_offset;
    return 0;
}
