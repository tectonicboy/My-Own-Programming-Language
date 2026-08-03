#define ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(symbols, iter, name, type, val, ptr)  \
    (iter) = (symbols)->find((name));                                          \
    if( (iter) == (symbols)->end() )                                           \
    [[unlikely]]                                                               \
    {                                                                          \
        (iter) =                                                               \
            ((symbols)->emplace((name), Symbol((type), (name), (val)))).first; \
    }                                                                          \
    (ptr) = &((iter)->second);

#define \
EMIT_ERR_IF_SYMBOL_ABSENT_OR_GET_PTR(symbols, iter, name, ptr, src_line)       \
    (iter) = (symbols)->find((name));                                          \
    if( (iter) == (symbols)->end() )                                           \
    [[unlikely]]                                                               \
    {                                                                          \
     std::cout << "Error: Symbol on RHS of assignment not initialized.\n";     \
     std:: cout << "Line: " << (src_line) << "\n";                             \
     std::abort();                                                             \
    }                                                                          \
    (ptr) = &((iter)->second);


#define VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(tok_arr, cursor, N) \
    if((tok_arr)->size() - ((cursor) + 1) < (N))               \
    [[unlikely]]                                               \
    {                                                          \
        std::cout << "Error: Incomplete program.\n";           \
        std::abort();                                          \
    }

/*----------------------------------------------------------------------------*/

/* The Parser and Parsing Orchestrator classes. */

/* TODO: Add to the ParsingOrchestrator class a counter for added symbols. */
/*       This counter will be passed down to the Parser objects so the
 *       parameterized macro that calls .emplace() on the Symbol Table
 *       (invoked by some of the Parser's AST geneating functions)
 *       will be able to update it in a thread safe way. This parser-local
 *       counter should be added to the grand total symbol count held
 *       by the ParsingOrchestrator, just like the count of all other
 *       things, like AST Nodes, entries in the statement directory, etc.
 */
class ParsingOrchestrator
{
public:

    /* Receives these from the Lexer. */
    std::vector<Token> token_array;
    Code_Block_Directory code_block_directory;

    /* Receives this from the top-level compilation driver. */
    std::vector<std::vector<size_t>> parsing_quotas;

    /* Brings into existence these new things. */
    size_t symbol_table_size;
    std::unordered_map<std::string, Symbol> symbol_table;

    uint8_t* ast_arena;
    size_t   ast_arena_size;
    size_t   ast_arena_next_free_region_offset;
    size_t   ast_arena_used_bytes;

    size_t statement_directory_used_entries;
    Statement_Directory statement_directory;

    /* Constructor. */
    explicit ParsingOrchestrator
        (Code_Block_Directory&& code_block_dir_in,
         std::vector<Token>&& token_array_in,
         std::vector<std::vector<size_t>> parser_quotas_in)
    : token_array(std::move(token_array_in)),
      code_block_directory(std::move(code_block_dir_in)),
      parsing_quotas(parser_quotas_in),
      symbol_table_size(10'000),
      ast_arena_size(100'000),
      ast_arena_next_free_region_offset(0),
      ast_arena_used_bytes(0),
      statement_directory(Statement_Directory(0, false))
    {
        symbol_table.reserve(symbol_table_size);

        ast_arena = (uint8_t*)malloc(ast_arena_size);
        if(ast_arena == NULL)
        {
            std::cout << "Internal compiler error: "
                         "Allocating the AST arena failed.\n";
            perror("errno: ");
            std::abort();
        }
        memset(ast_arena, 0x00, ast_arena_size);
    }

    uint8_t spawn_parser(std::vector<size_t> parsing_quota);
};

/* Each parser must have:
 *
 *      - A pointe to the Symbol Table object,
 *
 *      - A pointer to the Aux Code Block Directory object,
 *      - A pointer to a vector of which entries to parse the statements of,
 *
 *      - A pointer to the Token array object,
 *      - Pointer to current cursor to start reading at. Returned updated,
 *
 *      - A pointer to A FREE REGION OF the AST Memory Arena,
 *      - size_t: Available Arena memory (read-only),
 *      - size_t pointer: Passed as 0. Returns as: amount of USED Arena memory,
 *
 *      - A pointer to the Auxilliary Code Block Statement Directory object,
 *      - size_t: Number of available ACBSD entries. (read-only),
 *      - size_t: Next free ACBSD entry. (read-only),
 *      - size_t pointer: Passed as 0. Returns as: count of used ACBSD entries.
 */
class Parser {

public:
    std::unordered_map<std::string, Symbol>* symbol_table;
    Code_Block_Directory* code_block_directory;
    std::vector<size_t>* which_blocks_to_parse;
    std::vector<Token>* token_array;
    uint8_t*     ast_arena_free_region;
    const size_t available_arena_bytes;
    size_t*      used_arena_bytes;
    Statement_Directory* statement_directory;

    explicit
    Parser( std::unordered_map<std::string, Symbol>* symbol_table_in,
            Code_Block_Directory* code_block_dir_in,
            std::vector<size_t>* code_blocks_to_parse_in,
            std::vector<Token>* token_array_in,
            uint8_t* arena_region_in,
            const size_t avail_arena_bytes_in,
            size_t* nr_used_arena_bytes_in,
            Statement_Directory* statement_dir_in)
    : symbol_table(symbol_table_in),
      code_block_directory(code_block_dir_in),
      which_blocks_to_parse(code_blocks_to_parse_in),
      token_array(token_array_in),
      ast_arena_free_region(arena_region_in),
      available_arena_bytes(avail_arena_bytes_in),
      used_arena_bytes(nr_used_arena_bytes_in),
      statement_directory(statement_dir_in)
    {}

    uint8_t parse_blocks();
    uint8_t parse_statements(size_t* start_token_cursor, size_t block_dir_ix);

    uint8_t parse_statement
                    (size_t*  token_cursor,     uint8_t* ast_arena_region_ptr,
                     size_t   bytes_available,  size_t*  bytes_used,
                     size_t   codeblock_dir_ix, bool*    last_statement_seen,
                     size_t*  statement_wr_offset_after_alignment,
                     bool*    statement_dir_entry_adding);

    uint8_t parse_assignment_statement
            (size_t*      token_cursor,    uint8_t* ast_arena_region_ptr,
             const size_t bytes_available, size_t*  bytes_used,
             size_t*      this_node_wr_offset);

    uint8_t parse_bin_op_expr
            (size_t*      token_cursor,    uint8_t* ast_arena_region_ptr,
             const size_t bytes_available, size_t*  bytes_used,
             size_t*      new_node_wr_offset);

};

uint8_t
ParsingOrchestrator::spawn_parser(std::vector<size_t> parsing_quota)
{
    uint8_t ret;

    Parser my_parser = Parser(&symbol_table, &code_block_directory,
                              &parsing_quota, &token_array,
                              ast_arena, ast_arena_size,
                              &ast_arena_used_bytes, &statement_directory);

    ret = my_parser.parse_blocks();


    if(ret) [[unlikely]]
    {
        std::cout << "\nParsing FAILED!\n";
        return 1;
    }
    else
        std::cout << "\nParsing was a SUCCESS!\n";

    std::cout << "AST mem arena used bytes  : " << ast_arena_used_bytes << "\n";
    std::cout << "Statement dir used entries: "
              << this->statement_directory.size() << "\n";
    std::cout << "NEW next AST Arena free region offset: "
              << ast_arena_next_free_region_offset << "\n";
    std::cout << "\n\n";

    return 0;
}

/* HUGE BUGFIX TODO: When checking whether the AST Arena has enough memory
 *                   for us to construct the new AST Node object in it,
 *                   check for wr_offset + sizeof(AST_Object) and remove
 *                   the second check. Right now we write the object, move the
 *                   wr_offset AND ONLY THEN check if there was enough memory
 *                   to write the object. BUG!
 */
uint8_t Parser::parse_bin_op_expr(size_t*      token_cursor,
                                  uint8_t*     ast_arena_region_ptr,
                                  const size_t bytes_available,
                                  size_t*      bytes_used,
                                  size_t*      new_node_wr_offset)
{
    Symbol* symbol_ptr = nullptr;
    std::unordered_map<std::string, Symbol>::iterator symbol_table_iterator;
    AST_Node_Expression* rhs_expr_node_ptr = nullptr;
    AST_Node_Expression* lhs_expr_node_ptr = nullptr;
    std::string bin_operator;
    size_t cursor = *token_cursor;
    std::cout << "Entered CALL to BinOp parser with token cursor: "
              << cursor << "\n";
    std::cout << "Token: " << (*token_array)[cursor].token_value << "\n";
    size_t own_node_alignment = alignof(AST_Node_Expr_BinOp);
    size_t wr_offset = *bytes_used;
    size_t next_node_wr_offset;
    uint8_t ret;

    /* At the START of each Node-constructing token processor function, it
     * reserves enough bytes at the current offset in the AST Arena for its
     * own Node (+ any alignment bytes before that), then at the END of the
     * function it returns there to construct it now that all necessary fields
     * have been obtained.
     */

    while( ((uintptr_t)(ast_arena_region_ptr + wr_offset)) % own_node_alignment)
        ++wr_offset;

    *new_node_wr_offset = wr_offset;
    wr_offset += sizeof(AST_Node_Expr_BinOp);
    if(wr_offset > bytes_available) [[unlikely]] { return 1; }

    /* Grammar:
     *
     * Expression      ::= INT_LITERAL | IDENTIFIER | BinaryOperation
     * BinaryOperation ::= "(" Expression BinaryOperator Expression ")"
     * BinaryOperator  ::= "+" | "-" | "*" | "/"
     *
     * So:
     *
     * PART I: At 1st token of LHS. If '(', call here recursively. If LITERAL
     *         or IDENTIFIER, parse it, bump token cursor, move to PART II.
     *
     * PART II: Check the token is a valid operator. If so, add it as a
     *          std::string in the AST_Node_Expr_BinOp object this function
     *          is constructing, bump token cursor, move to PART III.
     *
     * PART III: At 1st token of RHS. If '(', call here recursively. If LITERAL
     *           or IDENTIFIER, parse it, bump token cursor. Check closing
     *           parenthesis of THIS binay operation.
     *
     * DO NOT CHECK FOR SEMICOLONS HERE. Statement syntax parser does that.
     */

    /* Part I. */

    /* Checking whether we even have X tokens ahead of us before peeking
     * at them is recurring. Find a way to factor it out more elegantly.
     */
    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(token_array, cursor, 1)

    if((*token_array)[cursor + 1].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        ++cursor;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }

        lhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    else if((*token_array)[cursor + 1].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        EMIT_ERR_IF_SYMBOL_ABSENT_OR_GET_PTR(symbol_table, symbol_table_iterator,
                                         std::string((*token_array)[cursor + 1].token_value),
                                         symbol_ptr, (*token_array)[cursor + 1].token_line_in_src)
        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset))
         % alignof(AST_Node_Expr_Identifier))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        lhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_Identifier(symbol_ptr);

        wr_offset += sizeof(AST_Node_Expr_Identifier);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the two we processed, to the BinOp sign. */
        cursor += 2;
    }
    else if((*token_array)[cursor + 1].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset)) %
         alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        lhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
                (std::stoull(std::string((*token_array)[cursor + 1].token_value)));

        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the two we processed, to the BinOp sign. */
        cursor += 2;
    }

    /* PART II. */

    /* Process the sign, add it to this BinOp object and move cursor by one. */
    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(token_array, cursor, 1)
    if((*token_array)[cursor].token_type_ix != TOKEN_TYPE_OPERATOR)
    {
        std::cout << "\nSyntax error: Sign of binary operation is invalid.\n"
                  << "Line: " << (*token_array)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    std::cout << "Adding BinOp sign: " << (*token_array)[cursor].token_value << "\n";
    bin_operator = std::string((*token_array)[cursor].token_value);
    ++cursor;

    /* PART III. */

    /* Exactly the same code as PART I. Parsing an Expression. But PART III
     * also checks for the closing parenthesis of THIS binary operation.
     * TODO: Factor the repeating code out somewhere.
     */

    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(token_array, cursor, 1)

    if((*token_array)[cursor].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }

        rhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    else if((*token_array)[cursor].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        EMIT_ERR_IF_SYMBOL_ABSENT_OR_GET_PTR(symbol_table, symbol_table_iterator,
                                             std::string((*token_array)[cursor].token_value),
                                             symbol_ptr, (*token_array)[cursor + 1].token_line_in_src)

        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset))
         % alignof(AST_Node_Expr_Identifier))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_Identifier(symbol_ptr);

        wr_offset += sizeof(AST_Node_Expr_Identifier);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the token we processed, to the close paren. */
        cursor += 1;
    }
    else if((*token_array)[cursor].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset)) %
         alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
                (std::stoull(std::string((*token_array)[cursor].token_value)));

        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the token we processed, to the close paren. */
        cursor += 1;
    }

    /* Last part of PART III: Check the closing paren. Don't check semicolon. */

    if((*token_array)[cursor].token_type_ix != TOKEN_TYPE_CLOSE_PAREN)
    [[unlikely]]
    {
        std::cout << "\nSyntax error: Missing closing parenthesis.\n"
                  << "Line: " << (*token_array)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    ++cursor;

    /* At the end of EACH Node-constructing token processor function, once all
     * the parts of its Node object are available, it returns to the reserved
     * space of sizeof(that_Node) (which is given by the untouched bytes_used
     * pointer handed down to the function from whoever called it), that it
     * reserved at the START of the function and constructs its object there.
     */
    new (ast_arena_region_ptr + (*new_node_wr_offset))
        AST_Node_Expr_BinOp(lhs_expr_node_ptr, rhs_expr_node_ptr, bin_operator);

    *token_cursor = cursor;
    *bytes_used = wr_offset;
    return 0;
}

/* Assignment statement parsing.
 *
 * Grammar: AssignmentStatement ::= IDENTIFIER "=" Expression ";"
 *
 * Three possible syntax variants:
 *
 * IDENTIFIER = INT_LITERAL;
 * IDENTIFIER = IDENTIFIER;
 * IDENTIFIER = <BinaryOperation>;
 *
 * where:
 *
 * BinaryOperation ::= "(" Expression BinaryOperator Expression ")"
 * BinaryOperator  ::= "+" | "-" | "*" | "/"
 *
 * We can figure out which syntax variant it is by peeking at further tokens.
 */
uint8_t Parser::parse_assignment_statement(size_t*      token_cursor,
                                           uint8_t*     ast_arena_region_ptr,
                                           const size_t bytes_available,
                                           size_t*      bytes_used,
                                           size_t*      this_node_wr_offset)
{
    std::unordered_map<std::string, Symbol>::iterator symbol_table_iterator;
    Symbol* symbol_ptr = nullptr;
    Symbol* lhs_symbol_ptr = nullptr;
    AST_Node_Expression* rhs_expr_node_ptr = nullptr;
    AST_Node_Statement_Assignment* statement_node_ptr = nullptr;
    size_t cursor = *token_cursor;
    const size_t lhs_symbol_cursor = cursor;
    size_t wr_offset = *bytes_used;
    size_t next_node_wr_offset;
    uint8_t ret;

    /* Parse the RHS of the assignment. */

    /* Construct an AST Assignment Node object. LHS (symbol pointer) filled.
     * RHS (Expression) starting out as a NULL pointer for now. After we see
     * what the RHS looks like, called the proper processor functions to emit
     * the AST Node(s) for it, we will get back a pointer to this Expression
     * AST Node object so we can complete the initialization of the Statement
     * AST Node object with it.
     */

    /* Reserve the space here for an assignment statement AST Node. */

    while(((uintptr_t)(ast_arena_region_ptr + wr_offset))
     % alignof(AST_Node_Statement_Assignment))
        ++wr_offset;
    if(wr_offset > bytes_available) [[unlikely]] { return 1; }

    *this_node_wr_offset = wr_offset;

    wr_offset += sizeof(AST_Node_Statement_Assignment);
    if(wr_offset > bytes_available) [[unlikely]] { return 1; }

    /* Syntax case 1, RHS Node is this object: AST_Node_Expr_UINT64_Literal. */

    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(token_array, cursor, 2)

    if((*token_array)[cursor + 2].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while(((uintptr_t)(ast_arena_region_ptr + wr_offset)) % alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
               (std::stoull(std::string((*token_array)[cursor + 2].token_value)));
        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the three we just processed. */
        cursor += 3;
    }
    /* Syntax case 2, RHS Node is this object: AST_Node_Expr_Identifier. */
    else if((*token_array)[cursor + 2].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        EMIT_ERR_IF_SYMBOL_ABSENT_OR_GET_PTR
            (symbol_table, symbol_table_iterator,
             std::string((*token_array)[cursor + 2].token_value),
             symbol_ptr, (*token_array)[cursor + 1].token_line_in_src)
        /* Align if needed. */
        while(((uintptr_t)(ast_arena_region_ptr + wr_offset))
         % alignof(AST_Node_Expr_Identifier))
        {
            ++wr_offset;
        }
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_Identifier(symbol_ptr);

        wr_offset          += sizeof(AST_Node_Expr_Identifier);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the three we processed. */
        cursor += 3;
    }
    /* Syntax case 3. */
    else if((*token_array)[cursor + 2].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        cursor += 2;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }
        rhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    /* Semicolon check is last, independent of assignment syntax type. */
    if((*token_array)[cursor].token_type_ix != TOKEN_TYPE_SEMICOLON)
    [[unlikely]]
    {
        std::cout
             << "\n\nSyntax error: Missing semicolon at end of assignment.\n"
             << "Line: " << (*token_array)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    ++cursor;

    /* A pointer to a Symbol object is needed as the LHS data member.
     * If present in the Symbol Table, add a pointer to it. If not,
     * construct it in the Symbol Table and then add the pointer to it.
     *
     * We add the LHS symbol of the assignment statement here because
     * if we do it at the start of this function, buggy code like this
     * will not be caught:
     *
     * x = x + 5;
     *
     * Where this is the first assignment to x. This is clearly invalid code.
     */
    ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(symbol_table, symbol_table_iterator,
                                     std::string((*token_array)[lhs_symbol_cursor].token_value),
                                     SYMBOL_KIND_UINT64, 0, lhs_symbol_ptr)

    statement_node_ptr = new (ast_arena_region_ptr + (*this_node_wr_offset))
       AST_Node_Statement_Assignment(lhs_symbol_ptr, rhs_expr_node_ptr);

    /* Update the Token cursor for upstream calls. */
    *token_cursor = cursor;
    *bytes_used   = wr_offset;
    return 0;
}

/* Top-level statement processor.
 *
 * This is the only function called by Parsers, which are spawned by a single
 * Parsing Orchestrator, whose job is to maintain the cache locality-friendly
 * memory arena used to store all Nodes of the constructed AST, along with any
 * necessary bookkeeping information.
 *
 * The Parsing Orchestrator generates the AST of a correctly written program. It
 * also generates the Symbol Table for the program and the Statement Directory
 * serving as a contiguous array containing the root AST Node of each Code Block
 * in the written source code, containing indices to each statement's AST Node
 * in the AST Arena, thereby completing the full AST.
 */
uint8_t Parser::parse_statement
                       (size_t* token_cursor,     uint8_t* ast_arena_region_ptr,
                        size_t  bytes_available,  size_t*  bytes_used,
                        size_t  codeblock_dir_ix, bool*    last_statement_seen,
                        size_t* statement_wr_offset_after_alignment,
                        bool* statement_dir_entry_adding)
{
    size_t cursor = *token_cursor;
    size_t wr_offset = *bytes_used;
    *statement_dir_entry_adding = false;

    /* The grammar is simple enough that the first token of the statement
     * reveals exactly what type of statement it is, depending on the Token type
     * so we can call the respective Statement Processor Function here. For
     * statements that don't add an AST Node, we parse them here without having
     * a special processor function for them, for example BLOCK_START.
     */

    /* Which token type? */
    switch((*token_array)[cursor].token_type_ix)
    {
    case TOKEN_TYPE_KEYWORD:
    {
        /* Which keyword? */
        if( (*token_array)[cursor].token_value
                == reserved_keyword_strings[KEYWORD_BLOCK_START] )
        {
            ++cursor;
            /* Which code block type are we starting? */
            if( (*token_array)[cursor].token_value
                    == reserved_keyword_strings[KEYWORD_PROGRAM] )
            {
                (*code_block_directory)[codeblock_dir_ix]
                    .code_block_type_index = CODE_BLOCK_TYPE_PROGRAM;
                ++cursor;
            }
            else
            {
                std::cout
                     << "\n\n"
                     << "Syntax error: Starting an invalid Code Block type.\n"
                     << "Line: " << (*token_array)[cursor].token_line_in_src
                     << "\n\n";
                std::abort();
            }
        }
        else if( (*token_array)[cursor].token_value
                    == reserved_keyword_strings[KEYWORD_BLOCK_END] )
        {
            ++cursor;
            *last_statement_seen = true;
        }
        else
        {
            std::cout
                 << "\n\n"
                 << "Unexpected error: Invalid keyword.\n"
                 << "Line: " << (*token_array)[cursor].token_line_in_src
                 << "\nThis should never happen. Must be investigated.\n\n";
            std::abort();
        }

        break;
    }
    /* This starts an assignment statement. */
    case TOKEN_TYPE_IDENTIFIER:
    {
        /* Quick syntax checks then call the processor function. */

        /* At least 4 more tokens are needed for a minimal assignment:
         * identifier = identifier ; BLOCK_END
         * If the token array ends before that, the program is incomplete.
         */
        VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(token_array, cursor, 4)
        /* Check for the equals sign. */
        if((*token_array)[cursor + 1].token_value != "=")
        [[unlikely]]
        {
            std::cout << "\n\n"
                      << "Syntax error: Assignment started but no '=' found.\n"
                      << "Line: " << (*token_array)[cursor + 1].token_line_in_src
                      << "\n\n";
            std::abort();
        }
        /* Check for what comes after the equals sign. */
        if(  (*token_array)[cursor + 2].token_type_ix != TOKEN_TYPE_IDENTIFIER
          && (*token_array)[cursor + 2].token_type_ix != TOKEN_TYPE_NUM_LITERAL_UINT
          && (*token_array)[cursor + 2].token_type_ix != TOKEN_TYPE_OPEN_PAREN)
        [[unlikely]]
        {
            std::cout
                 << "\n\n"
                 << "Syntax error: You have something wrong after the equals\n"
                 << "              sign in an assignment. Only 3 things are\n"
                 << "              allowed after the =, which are: Literal,\n"
                 << "              Identifier or '(' for binary operations.\n"
                 << "Line: " << (*token_array)[cursor + 2].token_line_in_src
                 << "\n\n";
            std::abort();
        }

        /* Start of assignment statement looks good. Parse it. */
        parse_assignment_statement(&cursor, ast_arena_region_ptr,
                                   bytes_available, &wr_offset,
                                   statement_wr_offset_after_alignment);

        *statement_dir_entry_adding = true;
        break;
    }
    } /* outer switch end. */
    *token_cursor = cursor;
    *bytes_used   = wr_offset;
    return 0;
}

uint8_t Parser::parse_statements(size_t* start_token_cursor,
                                 size_t  block_dir_ix)
{
    bool    last_statement_seen = false;
    bool    statement_dir_entry_adding;
    size_t  arena_offset_to_statement;
    uint8_t ret;
    size_t  statement_ix = 0;

    while(last_statement_seen == false)
    {
        ret = parse_statement(start_token_cursor,    ast_arena_free_region,
                              available_arena_bytes, used_arena_bytes,
                              block_dir_ix,          &last_statement_seen,
                              &arena_offset_to_statement,
                              &statement_dir_entry_adding);

        if(ret) [[unlikely]] { return 1; }

        if(statement_dir_entry_adding)
        {
            (*statement_directory).emplace_back
              (block_dir_ix, statement_ix, arena_offset_to_statement);
            ++statement_ix;
        }
    }
    return 0;
}

uint8_t Parser::parse_blocks()
{
    uint8_t ret;
    size_t start_cursor;

    for(size_t i = 0; i < which_blocks_to_parse->size(); ++i)
    {
        start_cursor = (*code_block_directory)
                        [(*which_blocks_to_parse)[i]].start_token_index;

        ret = parse_statements(&start_cursor, (*which_blocks_to_parse)[i]);

        if(ret) [[unlikely]]
        {
            std::cout << "[ERR] parse_statements() returned 1. Out of mem.\n\n";
            std::abort();
        }
    }

    std::cout << "\n[OK] Parsing successful!\n\n";
    return 0;
}
