/* The IR Generator and IR Generation Orchestrator classes. */

/* Gives jobs to one or more IR Generators by giving them quotas for which
 * Code Blocks' statement AST Nodes to emit IR instructions for.
 */
class IR_Generation_Orchestrator
{

public:
    /* Receives these from a Parsing_Orchestrator: */
    MEM_Arena ast_arena;
    Statement_Directory statement_dir;

    /* Brings into existence these new things: */
    MEM_Arena IR_instructions_arena;
    IR_Instructions_Directory IR_instructions_dir;

    /* Receives this from the top-level compilation driver: */
    std::vector<std::vector<size_t>> IR_generation_quotas;

    /* Constructor */
    explicit IR_Generation_Orchestrator
        (MEM_Arena&& ast_arena_in,
         Statement_Directory&& statement_dir_in,
         std::vector<std::vector<size_t>>&& IR_gen_quotas_in)
    : ast_arena(std::move(ast_arena_in)),
      statement_dir(std::move(statement_dir_in)),
      IR_instructions_arena(MEM_Arena(std::string("IR Instructions Arena"))),
      IR_instructions_dir(IR_Instructions_Directory(0, 0)),
      IR_generation_quotas(std::move(IR_gen_quotas_in)) {}

    uint8_t spawn_IR_generator(std::vector<size_t> IR_generation_quota);
};

class IR_Generator
{
private:
    /* Receives from an IR Generation Orchestrator: */
    MEM_Arena* ast_arena;
    Statement_Directory* statement_dir;
    MEM_Arena* IR_instructions_arena;
    IR_Instructions_Directory* IR_instructions_dir;
    std::vector<size_t> IR_generation_quota;

    size_t IR_intermediates_emitted;
    size_t count_u64_literals_seen;
    size_t IR_instructions_emitted;
    size_t count_statements_it_emitted_IR_for;

    const size_t        encountered_u64_literals_array_init_size;
    size_t              encountered_u64_literals_array_cur_size;
    std::vector<size_t> encountered_u64_literals_array;

public:
    /* Constructor */
    explicit IR_Generator
        (MEM_Arena* ast_arena_ptr_in, Statement_Directory* statement_dir_ptr_in,
         MEM_Arena* IR_instructions_arena_ptr_in,
         IR_Instructions_Directory* IR_instructions_dir_in,
         std::vector<size_t> IR_generation_quota_in)
    : ast_arena(ast_arena_ptr_in),
      statement_dir(statement_dir_ptr_in),
      IR_instructions_arena(IR_instructions_arena_ptr_in),
      IR_instructions_dir(IR_instructions_dir_in),
      IR_generation_quota(IR_generation_quota_in), IR_intermediates_emitted(0),
      count_u64_literals_seen(0), IR_instructions_emitted(0),
      count_statements_it_emitted_IR_for(0),
      encountered_u64_literals_array_init_size(1'000),
      encountered_u64_literals_array_cur_size(0)
    {
        encountered_u64_literals_array.reserve
            (encountered_u64_literals_array_init_size);
    }

    uint8_t emit_IR(void);

private:
    uint8_t emit_IR_for_assignment
                (AST_Node_Statement_Assignment* stmt_node,
                 const size_t code_block_ix, const size_t statement_ix);

    /* If this function is called, at least one auxilliary IR instruction has to
     * be emitted to store the result of a nested binary operation. More of them
     * if the input BinOp AST Node contains more nested BinOps.
     */
    uint8_t emit_auxilliary_IR_for_nested_binop
            (const size_t code_block_ix, const size_t statement_ix,
             size_t* passed_insns_emitted_for_stmt, AST_Node_Expr_BinOp* binop);

    uint8_t emit_IR_insn_EQU
                (std::string lhs, std::string rhs, const size_t code_block_ix,
                 const size_t statement_ix, const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_ADD
                (std::string ir_insn_target, std::string ir_insn_operand1,
                 std::string ir_insn_operand2, const size_t code_block_ix,
                 const size_t statement_ix, const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_SUB
                (std::string ir_insn_target, std::string ir_insn_operand1,
                 std::string ir_insn_operand2, const size_t code_block_ix,
                 const size_t statement_ix, const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_MUL
                (std::string ir_insn_target, std::string ir_insn_operand1,
                 std::string ir_insn_operand2, const size_t code_block_ix,
                 const size_t statement_ix, const size_t ir_instruction_ix);

    uint8_t emit_IR_insn_DIV
                (std::string ir_insn_target, std::string ir_insn_operand1,
                 std::string ir_insn_operand2, const size_t code_block_ix,
                 const size_t statement_ix, const size_t ir_instruction_ix);

    inline uint8_t construct_IR_operand_from_u64_literal
      (const uint64_t val, std::string& operand_str, const size_t code_block_ix,
       const size_t statement_ix, size_t* insns_emitted_for_stmt);

    inline uint8_t emit_IR_binop_insn
     (const std::string sign_str, const std::string ir_insn_target,
      const std::string ir_insn_operand1, const std::string ir_insn_operand2,
      const size_t code_block_ix, const size_t statement_ix,
      const size_t insns_emitted_for_this_stmt);
};


/* The main compilation driver chooses which IR generation quota to process. */
uint8_t IR_Generation_Orchestrator::spawn_IR_generator
        (std::vector<size_t> selected_IR_generation_quota)
{
    uint8_t ret = 0;
    IR_Generator my_IR_generator
        (&(ast_arena), &(statement_dir), &(IR_instructions_arena),
         &(IR_instructions_dir), selected_IR_generation_quota);

    ret = my_IR_generator.emit_IR();
    if(ret) [[unlikely]] { return ret; }

    std::cout << "\n  ****  IR generation successful!  ****\n\n";
    std::cout << "IR Instructions emitted: "
              << IR_instructions_dir.size() << "\n";
    std::cout << "IR Arena bytes used: "
              << IR_instructions_arena.wr_offset << "\n";

    return ret;
}

/* Top-level driving function for emitting IR for a given quota of Code Blocks.
 *
 * This function simply goes over the input IR generation quota, which is just
 * Code Block indices to emit IR for, finds the entries in Statement Directory
 * of these Code Blocks, looks at the memory offset for each Statement AST Node,
 * again recorded in each Statement Directory entry, goes into the AST Arena
 * where each Statement AST Node lives and uses it to emit initial SSA IR code
 * for each source statement by calling its respective IR emitting function.
 *
 * The different scenarios that change how IR code is emitted for a source
 * statement, depending on its structure and type, are handled by the respective
 * IR emitting function.
 */
uint8_t IR_Generator::emit_IR(void)
{
    uint8_t ret = 0;
    bool end_of_statement_dir_reached = false;
    size_t curr_statement_dir_ix = 0;
    size_t curr_quota_block_ix;
    size_t curr_ast_arena_offset;
    size_t statements_we_emitted_IR_for = 0;
    AST_Node_Statement* cur_stmt_ast_node;

    /* Go over the quota entries and emit IR code for the given Code Blocks. */
    for(size_t i = 0; i < IR_generation_quota.size(); ++i)
    {
        /* Find this Code Block's first statement in the Statement Directory.
         * Go to its indicated offset into the AST Arena. Emit IR instruction(s)
         * for this Statement AST Node. Do the same for all next statement AST
         * Nodes until the Code Block index in the next Statement Directory
         * entry is not the one our quota told us to work on.
         */
        curr_quota_block_ix = IR_generation_quota[i];

        while( (*statement_dir)[curr_statement_dir_ix].code_block_ix
               != curr_quota_block_ix)
        {
            ++curr_statement_dir_ix;
            if(curr_statement_dir_ix == (*statement_dir).size())
            [[unlikely]]
            {
                std::cout << "IR_Generator::emit_IR : Reached end of ACBSD.\n";
                end_of_statement_dir_reached = true;
                break;
            }
        }
        if(end_of_statement_dir_reached == true) [[unlikely]] { break; }

        while( (*statement_dir)[curr_statement_dir_ix].code_block_ix
               == curr_quota_block_ix)
        {
            curr_ast_arena_offset = (*statement_dir)
                             [curr_statement_dir_ix].root_ast_node_arena_offset;

            cur_stmt_ast_node = (AST_Node_Statement*)
                                 (ast_arena->arena_ptr + curr_ast_arena_offset);

            if
            (cur_stmt_ast_node->statement_kind_ix == STATEMENT_KIND_ASSIGNMENT)
            {
                ret = emit_IR_for_assignment
                           ((AST_Node_Statement_Assignment*)cur_stmt_ast_node,
                             curr_quota_block_ix, statements_we_emitted_IR_for);
            }
            else [[unlikely]]
            {
                std::cout << "Internal compiler error: IR_Generator::emit_IR: "
                             "Invalid statement type in AST Node.\n";
                std::abort();

            }
            ++statements_we_emitted_IR_for;

            /* Go to next Statement Directory entry.
             * Make sure we haven't reached its end. */
            ++curr_statement_dir_ix;

            if(curr_statement_dir_ix == (*statement_dir).size())
            [[unlikely]]
            {
                std::cout << "IR_Generator::emit_IR : Reached end of ACBSD.\n";
                end_of_statement_dir_reached = true;
                break;
            }
        }
        if(end_of_statement_dir_reached == true) [[unlikely]] { break; }
    }

    if(end_of_statement_dir_reached == true)
        std::cout << "IR Generator: End of Statement Directory was reached.\n";

    std::cout << "[OK] IR Generator finished its work quota.\n"
                 "     Source statements IR code was emitted for: "
              << statements_we_emitted_IR_for << "\n";

    return ret;
}

inline uint8_t IR_Generator::construct_IR_operand_from_u64_literal
    (const uint64_t val, std::string& operand_str, const size_t code_block_ix,
     const size_t statement_ix, size_t* insns_emitted_for_stmt)
{
    bool    literal_has_already_been_encountered = false;
    size_t  i;
    uint8_t ret = 0;

    /* Look for the u64 literal in the array of already seen ones. */
    for(i = 0; i < encountered_u64_literals_array.size(); ++i)
    {
        if(val == encountered_u64_literals_array[i])
        {
            literal_has_already_been_encountered = true;
            break;
        }
    }
    operand_str = "%const_" + std::to_string(i);

    if(literal_has_already_been_encountered == false)
    {
        ret = emit_IR_insn_EQU(operand_str, std::to_string(val), code_block_ix,
                               statement_ix, *insns_emitted_for_stmt);
        if(ret) [[unlikely]] { return ret; }

        /* Place newly recorded literal in the array of already seen ones. */
        encountered_u64_literals_array.emplace_back(val);
        ++(*insns_emitted_for_stmt);
    }
    return ret;
}

inline uint8_t IR_Generator::emit_IR_binop_insn
    (const std::string sign_str, const std::string ir_insn_target,
     const std::string ir_insn_operand1, const std::string ir_insn_operand2,
     const size_t code_block_ix, const size_t statement_ix,
     const size_t insns_emitted_for_this_stmt)
{
    uint8_t ret;

    if(sign_str == "+")
        ret = emit_IR_insn_ADD
                (ir_insn_target, ir_insn_operand1, ir_insn_operand2,
                 code_block_ix, statement_ix, insns_emitted_for_this_stmt);

    else if(sign_str == "-")
        ret = emit_IR_insn_SUB
                (ir_insn_target, ir_insn_operand1, ir_insn_operand2,
                 code_block_ix, statement_ix, insns_emitted_for_this_stmt);

    else if(sign_str == "*")
        ret = emit_IR_insn_MUL
                (ir_insn_target, ir_insn_operand1, ir_insn_operand2,
                 code_block_ix, statement_ix, insns_emitted_for_this_stmt);

    else if(sign_str == "/")
        ret = emit_IR_insn_DIV
                (ir_insn_target, ir_insn_operand1, ir_insn_operand2,
                 code_block_ix, statement_ix, insns_emitted_for_this_stmt);

    return ret;
}

/* This function emits a single IR instruction for a single Assignment Statement
 * via its AST Node. Assignments from binary operations and from literals can
 * cause more IR instructions to be emitted beforehand, as each literal that
 * hasn't yet been encountered, as well as the result of each nested binary
 * operation result, get assigned to their own auxilliary IR variable.
 *
 * Three cases exist that have to be handled differently:
 *
 *  - Direct assignment from a literal: my_var = 5
 *
 *      1. Check whether this literal has an auxilliary IR variable already
 *         created for it by checking if it's in the array of already seen ones.
 *
 *      2. If found, use that literal's IR variable in the emitted instruction.
 *         Else, create a new auxilliary IR variable for the literal and use it.
 *
 *  - Direct assignment from another source variable: my_var = other_variable
 *
 *      1. Use the RHS source variable's current counter in Symbol MINUS ONE,
 *         as the RHS IR variable in the emitted IR instruction, like %b_1.
 *         MINUS ONE because the current counter is the NEXT VERSION of that
 *         source variable next time it is assigned to. No need to check for use
 *         before init, this was checked during AST generation and analysis.
 *
 *  - Assignment from a binary operation: SIMPLE: a = (b + 5)
 *                                        NESTED: a = ( (b * (c + 10)) + 5)
 *
 *      1. Recursively check whether the Expression in LHS and RHS of this
 *         binary operation is itself a binary operation. Keep going until
 *         it's not. Abstract base class for Expression has a data member
 *         holding the Expression Type, which determines what derived Expression
 *         object we are looking at, based on which we can use the correct
 *         pointer type when accessing the Expression AST Node object.
 *
 *      2. Create an intermediate IR variable to store the result of each
 *         nested binary operation.
 *
 *         Basically, each binary operation's result, starting from the deepest
 *         nested one (which by definition only has a Symbol as RHS and LHS),
 *         is stored in its own auxilliary IR variable, in order to maintain
 *         Static Single-Assignment Form. The newly created auxilliary IR
 *         variables are then used in the binary operations whose RHS/LHS was
 *         THAT nested binary operation, which the IR vaiable was created for,
 *         up the chain of recursive nested binary operations.
 *
 *  --------------------------------------------------------------------------
 *
 *      LAST STEP: Determine the LHS variable of the emitted IR instruction:
 *
 *         Check this LHS source variable's counter in its Symbol. Use current
 *         counter to produce the new LHS IR variable, e.g.: %a_2.
 *         Increment that source variable's counter in its Symbol object.*
 */
uint8_t
IR_Generator::emit_IR_for_assignment
    (AST_Node_Statement_Assignment* stmt_node, const size_t code_block_ix,
     const size_t statement_ix)
{
    uint8_t ret = 0;

    /* Locals: Bookkeeping. */
    size_t insns_emitted_for_this_stmt = 0;

    /* The rest: Function local temporaries for convenience. */

    /* Always used: */
    uint8_t assignment_rhs_expr_kind = stmt_node->rhs_expression->expr_kind_ix;
    std::string assignment_lhs_src_var = stmt_node->lhs_identifier->symbol_name;
    std::string ir_insn_target;
    std::string ir_insn_operand1;
    std::string sign_str;
    size_t name_mangle_ix;

    /* RHS of each assignment is an expression of one of these kinds: */
    AST_Node_Expr_BinOp*      rhs_expr_binop       = nullptr;
    AST_Node_Expr_Identifier* rhs_expr_identifier  = nullptr;

    /* May or may not have these. Assignments from literals don't have them. */
    uint64_t literal_val;
    std::string assignment_rhs_var1;
    std::string assignment_rhs_var2;

    /* Only have these if the assignment RHS is a BinOp. */
    std::string ir_insn_operand2;
    uint8_t binop_lhs_expr_kind;
    uint8_t binop_rhs_expr_kind;
    AST_Node_Expr_Identifier* binop_lhs_expr_identifier;
    AST_Node_Expr_Identifier* binop_rhs_expr_identifier;

    /* Look at assignment RHS: which of the 3 cases are we dealing with? */

    /* Case 1. Direct assignment from a literal, e.g.  my_var = 5 */
    if(assignment_rhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
    {
        literal_val = ((AST_Node_Expr_UINT64_Literal*)
                       (stmt_node->rhs_expression))->value;

        ret = construct_IR_operand_from_u64_literal
                   (literal_val, ir_insn_operand1, code_block_ix, statement_ix,
                    &insns_emitted_for_this_stmt);
        if(ret) [[unlikely]] { return ret; }

        name_mangle_ix = stmt_node->lhs_identifier->SSA_IR_mangle_counter;

        ir_insn_target =
               std::string("%") + assignment_lhs_src_var + std::string("_")
             + std::to_string(name_mangle_ix);

        ret = emit_IR_insn_EQU(ir_insn_target, ir_insn_operand1, code_block_ix,
                               statement_ix, insns_emitted_for_this_stmt);
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
        name_mangle_ix = rhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1;

        ir_insn_operand1 =   std::string("%") + assignment_rhs_var1
                           + std::string("_") + std::to_string(name_mangle_ix);

        name_mangle_ix = stmt_node->lhs_identifier->SSA_IR_mangle_counter;

        ir_insn_target =   std::string("%") + assignment_lhs_src_var
                         + std::string("_") + std::to_string(name_mangle_ix);

        ret = emit_IR_insn_EQU(ir_insn_target, ir_insn_operand1, code_block_ix,
                               statement_ix, insns_emitted_for_this_stmt);
        if(ret) [[unlikely]] { return ret; }

        ++insns_emitted_for_this_stmt;
        stmt_node->lhs_identifier->SSA_IR_mangle_counter += 1;
    }

    /* Case 3. Assignment from a (possibly multi-level) binary operation. */
    else if(assignment_rhs_expr_kind == EXPR_KIND_BIN_OPERATION)
    {
        /* Operand 1: might be a literal, a symbol, or a nested BinOp. */
        rhs_expr_binop      = (AST_Node_Expr_BinOp*)(stmt_node->rhs_expression);
        binop_lhs_expr_kind = rhs_expr_binop->lhs_expression->expr_kind_ix;

        /* if it's a literal: */
        if(binop_lhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
        {
            literal_val = ((AST_Node_Expr_UINT64_Literal*)
                           (rhs_expr_binop->lhs_expression))->value;

            ret = construct_IR_operand_from_u64_literal
                   (literal_val, ir_insn_operand1, code_block_ix, statement_ix,
                    &insns_emitted_for_this_stmt);
            if(ret) [[unlikely]] { return ret; }
        }
        /* if it's a source variable: */
        else if(binop_lhs_expr_kind == EXPR_KIND_IDENTIFIER)
        {
            binop_lhs_expr_identifier =
                (AST_Node_Expr_Identifier*)(rhs_expr_binop->lhs_expression);

            assignment_rhs_var1 =
                binop_lhs_expr_identifier->symbol->symbol_name;

            name_mangle_ix =
                binop_lhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1;

            ir_insn_operand1 =
                  std::string("%") + assignment_rhs_var1
                + std::string("_") + std::to_string(name_mangle_ix);
        }
        /* If operand 1 is itself a BinOp: */
        else if(binop_lhs_expr_kind == EXPR_KIND_BIN_OPERATION)
        {
            emit_auxilliary_IR_for_nested_binop
                (code_block_ix, statement_ix, &insns_emitted_for_this_stmt,
                 (AST_Node_Expr_BinOp*)rhs_expr_binop->lhs_expression);

            ir_insn_operand1 =   std::string("%_temp_")
                               + std::to_string(IR_intermediates_emitted - 1);
        }

        /* Operand 2: might be a literal, a symbol, or a nested BinOp. */
        binop_rhs_expr_kind = rhs_expr_binop->rhs_expression->expr_kind_ix;

        /* if it's a literal: */
        if(binop_rhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
        {
            literal_val = ((AST_Node_Expr_UINT64_Literal*)
                           (rhs_expr_binop->rhs_expression))->value;

            ret = construct_IR_operand_from_u64_literal
                   (literal_val, ir_insn_operand2, code_block_ix, statement_ix,
                    &insns_emitted_for_this_stmt);
            if(ret) [[unlikely]] { return ret; }
        }
        /* if it's a source variable: */
        else if(binop_rhs_expr_kind == EXPR_KIND_IDENTIFIER)
        {
            binop_rhs_expr_identifier =
                (AST_Node_Expr_Identifier*)(rhs_expr_binop->rhs_expression);

            assignment_rhs_var2 =
                binop_rhs_expr_identifier->symbol->symbol_name;

            name_mangle_ix =
                binop_rhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1;

            ir_insn_operand2 =
                  std::string("%") + assignment_rhs_var2
                + std::string("_") + std::to_string(name_mangle_ix);
        }
        /* If operand 2 is itself a BinOp: */
        else if(binop_rhs_expr_kind == EXPR_KIND_BIN_OPERATION)
        {
            ret = emit_auxilliary_IR_for_nested_binop
                    (code_block_ix, statement_ix, &insns_emitted_for_this_stmt,
                     (AST_Node_Expr_BinOp*)rhs_expr_binop->rhs_expression);
            if(ret) [[unlikely]] { return ret; }

            ir_insn_operand2 =   std::string("%_temp_")
                               + std::to_string(IR_intermediates_emitted - 1);
        }

        name_mangle_ix = stmt_node->lhs_identifier->SSA_IR_mangle_counter;

        ir_insn_target =   std::string("%") + assignment_lhs_src_var
                         + std::string("_") + std::to_string(name_mangle_ix);

        stmt_node->lhs_identifier->SSA_IR_mangle_counter += 1;

        /* Which sign does the BinOp have? */
        sign_str = rhs_expr_binop->binary_operator;

        ret = emit_IR_binop_insn
               (sign_str, ir_insn_target, ir_insn_operand1, ir_insn_operand2,
                code_block_ix, statement_ix, insns_emitted_for_this_stmt);
        if(ret) [[unlikely]] { return ret; }

        ++insns_emitted_for_this_stmt;
    }

    ++count_statements_it_emitted_IR_for;
    ++IR_instructions_emitted;
    return 0;
}

/* This function will emit 1 auxilliary IR instruction per call, storing the
 * result of the binary operation. If, inside the nested BinOp, one of the
 * sides is itself another nested BinOp, recursively call itself. Additional
 * IR instructions are emitted to store never-before-seen literals into IR
 * variables, and for further nest BinOps.
 */
uint8_t IR_Generator::emit_auxilliary_IR_for_nested_binop
            (const size_t code_block_ix,    const size_t statement_ix,
             size_t* passed_insns_emitted_for_stmt, AST_Node_Expr_BinOp* binop)
{
    uint8_t ret = 0;

    /* Local: For loop iterations. */

    /* Locals: Bookkeeping. */
    size_t insns_emitted_for_this_stmt = *passed_insns_emitted_for_stmt;

    /* The rest: Function local temporaries for convenience and readability. */

    /* Always used: */
    std::string ir_insn_target;
    std::string ir_insn_operand1;
    std::string ir_insn_operand2;
    std::string sign_str;
    uint8_t     binop_lhs_expr_kind;
    uint8_t     binop_rhs_expr_kind;
    size_t      name_mangle_ix;

    /* Have these only if a side of this BinOp is a source variable. */
    std::string binop_rhs_var;
    std::string binop_lhs_var;

    /* Need these when a side of the BinOp is a literal or an identifier. */
    AST_Node_Expr_Identifier*     binop_rhs_expr_identifier;
    AST_Node_Expr_Identifier*     binop_lhs_expr_identifier;

    /* BinOp LHS: 3 cases. */
    binop_lhs_expr_kind = binop->lhs_expression->expr_kind_ix;

    /* if it's a literal: */
    if(binop_lhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
    {
        ret = construct_IR_operand_from_u64_literal
              ( ((AST_Node_Expr_UINT64_Literal*)(binop->lhs_expression))->value,
                 ir_insn_operand1, code_block_ix, statement_ix,
                 &insns_emitted_for_this_stmt);
        if(ret) [[unlikely]] { return ret; }
    }
    /* if it's a source variable: */
    else if(binop_lhs_expr_kind == EXPR_KIND_IDENTIFIER)
    {
        binop_lhs_expr_identifier
            = (AST_Node_Expr_Identifier*)(binop->lhs_expression);

        binop_lhs_var = binop_lhs_expr_identifier->symbol->symbol_name;

        name_mangle_ix =
            binop_lhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1;

        ir_insn_operand1 =   std::string("%") + binop_lhs_var
                           + std::string("_")
                           + std::to_string(name_mangle_ix);
    }
    /* if it's a nested binop: */
    else if(binop_lhs_expr_kind == EXPR_KIND_BIN_OPERATION)
    {
        ret = emit_auxilliary_IR_for_nested_binop
                (code_block_ix, statement_ix,
                 &insns_emitted_for_this_stmt,
                 (AST_Node_Expr_BinOp*)binop->lhs_expression);
        if(ret) [[unlikely]] { return ret; }

        ir_insn_operand1 =   std::string("%_temp_")
                           + std::to_string(IR_intermediates_emitted - 1);
    }

    /*------------------------------------------------------------------------*/

    /* BinOp RHS: 3 cases. */
    binop_rhs_expr_kind = binop->rhs_expression->expr_kind_ix;

    /* if it's a literal: */
    if(binop_rhs_expr_kind == EXPR_KIND_UINT64_LITERAL)
    {
        ret = construct_IR_operand_from_u64_literal
              ( ((AST_Node_Expr_UINT64_Literal*)(binop->rhs_expression))->value,
                 ir_insn_operand2, code_block_ix, statement_ix,
                 &insns_emitted_for_this_stmt);
        if(ret) [[unlikely]] { return ret; }
    }

    /* if it's a source variable: */
    else if(binop_rhs_expr_kind == EXPR_KIND_IDENTIFIER)
    {
        binop_rhs_expr_identifier
            = (AST_Node_Expr_Identifier*)binop->rhs_expression;

        binop_rhs_var = binop_rhs_expr_identifier->symbol->symbol_name;

        name_mangle_ix =
            binop_rhs_expr_identifier->symbol->SSA_IR_mangle_counter - 1;

        ir_insn_operand2 =
              std::string("%") + binop_rhs_var
            + std::string("_") + std::to_string(name_mangle_ix);
    }
    /* if it's a nested binop: */
    else if(binop_rhs_expr_kind == EXPR_KIND_BIN_OPERATION)
    {
        ret = emit_auxilliary_IR_for_nested_binop
                (code_block_ix, statement_ix,
                 &insns_emitted_for_this_stmt,
                 (AST_Node_Expr_BinOp*)binop->rhs_expression);
        if(ret) [[unlikely]] { return ret; }

        ir_insn_operand2 =   std::string("%_temp_")
                           + std::to_string(IR_intermediates_emitted - 1);
    }

    /* OK. We have IR operands 1 and 2. Now construct the IR instruction
     * target and emit the auxilliary IR instruction based on the sign.
     */
    ir_insn_target =   std::string("%_temp_")
                     + std::to_string(IR_intermediates_emitted);

    /* Which sign does the BinOp have? */
    sign_str = binop->binary_operator;

    ret = emit_IR_binop_insn
      (sign_str, ir_insn_target, ir_insn_operand1, ir_insn_operand2,
       code_block_ix, statement_ix, insns_emitted_for_this_stmt);
    if(ret) [[unlikely]] { return ret; }

    ++insns_emitted_for_this_stmt;
    ++IR_intermediates_emitted;
    *passed_insns_emitted_for_stmt = insns_emitted_for_this_stmt;
    return 0;
}

uint8_t IR_Generator::emit_IR_insn_EQU
            (std::string lhs, std::string rhs, const size_t code_block_ix,
             const size_t statement_ix, const size_t ir_instruction_ix)
{
    /* Place the new IR Instruction object in the IR Instructions Arena. */
    size_t offset = IR_instructions_arena->add_entry<ir_insn_equate>(lhs, rhs);

    /* Add an entry in the IR Instructions Directory for it. */
    IR_instructions_dir->emplace_back
       (code_block_ix, statement_ix, ir_instruction_ix, offset, IR_INSN_EQUATE);

    return 0;
}

uint8_t IR_Generator::emit_IR_insn_ADD
            (std::string  ir_insn_target,   std::string ir_insn_operand1,
             std::string  ir_insn_operand2, const size_t code_block_ix,
             const size_t statement_ix,     const size_t ir_instruction_ix)
{
    /* Place the new IR Instruction object in the IR Instructions Arena. */
    size_t offset = IR_instructions_arena->add_entry<ir_insn_add>
        (ir_insn_operand1, ir_insn_operand2, ir_insn_target);

    /* Add an entry in the IR Instructions Directory for it. */
    IR_instructions_dir->emplace_back
        (code_block_ix, statement_ix, ir_instruction_ix, offset, IR_INSN_ADD);

    return 0;
}

uint8_t IR_Generator::emit_IR_insn_SUB
            (std::string  ir_insn_target,   std::string ir_insn_operand1,
             std::string  ir_insn_operand2, const size_t code_block_ix,
             const size_t statement_ix,     const size_t ir_instruction_ix)
{
    /* Place the new IR Instruction object in the IR Instructions Arena. */
    size_t offset = IR_instructions_arena->add_entry<ir_insn_sub>
                           (ir_insn_operand1, ir_insn_operand2, ir_insn_target);

    /* Add an entry in the IR Instructions Directory for it. */
    IR_instructions_dir->emplace_back
        (code_block_ix, statement_ix, ir_instruction_ix, offset, IR_INSN_SUB);

    return 0;
}

uint8_t IR_Generator::emit_IR_insn_MUL
            (std::string  ir_insn_target,   std::string ir_insn_operand1,
             std::string  ir_insn_operand2, const size_t code_block_ix,
             const size_t statement_ix,     const size_t ir_instruction_ix)
{
    /* Place the new IR Instruction object in the IR Instructions Arena. */
    size_t offset = IR_instructions_arena->add_entry<ir_insn_mul>
                           (ir_insn_operand1, ir_insn_operand2, ir_insn_target);

    /* Add an entry in the IR Instructions Directory for it. */
    IR_instructions_dir->emplace_back
        (code_block_ix, statement_ix, ir_instruction_ix, offset, IR_INSN_MUL);

    return 0;
}

uint8_t IR_Generator::emit_IR_insn_DIV
            (std::string  ir_insn_target,   std::string ir_insn_operand1,
             std::string  ir_insn_operand2, const size_t code_block_ix,
             const size_t statement_ix,     const size_t ir_instruction_ix)
{
    /* Place the new IR Instruction object in the IR Instructions Arena. */
    size_t offset = IR_instructions_arena->add_entry<ir_insn_div>
                           (ir_insn_operand1, ir_insn_operand2, ir_insn_target);

    /* Add an entry in the IR Instructions Directory for it. */
    IR_instructions_dir->emplace_back
        (code_block_ix, statement_ix, ir_instruction_ix, offset, IR_INSN_DIV);

    return 0;
}
