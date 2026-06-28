#define ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(symbols, iter, name, type, val, ptr)  \
    (iter) = (symbols)->find((name));                                          \
    if( (iter) == (symbols)->end() )                                           \
    [[unlikely]]                                                               \
    {                                                                          \
        (iter) =                                                               \
            ((symbols)->emplace((name), Symbol((type), (name), (val)))).first; \
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

/* The Parser and ParsingOrchestrator classes. */

/* TODO: Add to the ParsingOrchestrator class a counter for added symbols. */
/*       This counter will be passed down to the Parser objects so the
 *       parameterized macro that calls .emplace() on the Symbol Table
 *       (invoked by some of the Parser's AST geneating functions)
 *       will be able to update it in a thread safe way. This parser-local
 *       counter should be added to the grand total symbol count held
 *       by the ParsingOrchestrator, just like the count of all other
 *       things, like AST Nodes, entries in the statement directory, etc.
 */
class ParsingOrchestrator {

public:

    std::unordered_map<std::string, Symbol> Symbol_Table;
    size_t symbol_table_size;

    std::vector<Code_Block> code_block_directory;
    std::vector<std::vector<size_t>> parsing_quotas;

    std::vector<Token>      Tokens;

    alignas(64) uint8_t* ast_arena;
    size_t   ast_arena_size;
    size_t   ast_arena_next_free_region_offset;
    size_t   ast_arena_used_bytes;

    size_t statement_directory_size;
    size_t statement_directory_next_free_entry;
    size_t statement_directory_used_entries;
    std::vector<std::tuple<size_t, size_t, size_t>> statement_directory;

    explicit ParsingOrchestrator(std::vector<Code_Block>&& code_block_dir,
                                 std::vector<Token>&&      token_array,
                                 std::vector<std::vector<size_t>> parser_quotas)
    :symbol_table_size(10'000),
     code_block_directory(std::move(code_block_dir)),
      parsing_quotas(parser_quotas),
      Tokens(std::move(token_array)),
      ast_arena_size(100'000),
      ast_arena_next_free_region_offset(0),
      ast_arena_used_bytes(0),
      statement_directory_size(10'000),
      statement_directory_next_free_entry(0),
      statement_directory_used_entries(0),
      statement_directory(statement_directory_size,
                          std::make_tuple((size_t)0, (size_t)0, (size_t)0))
    {
        Symbol_Table.reserve(symbol_table_size);

        ast_arena = (uint8_t*)malloc(ast_arena_size);
        if(ast_arena == NULL)
        {
            std::cout << "Internal error: Allocating the AST arena failed.\n";
            perror("errno: ");
            std::abort();
        }
        memset(ast_arena, 0x00, ast_arena_size);
    }

    uint8_t spawn_parser(std::vector<size_t> parsing_quota);
};

/*
 * TODO: Non-critical, but good for maintainability: Instead of having
 *       a vector<tuple<size_t, size_t, size_t>> for the ACBSD, consider
 *       making each entry a struct object with 3 named data members instead
 *       of 3 size_t's.
 */

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
    std::unordered_map<std::string, Symbol>* Symbol_Table;

    std::vector<Code_Block>* aux_code_block_directory;
    std::vector<size_t>* which_blocks_to_parse;

    std::vector<Token>* Tokens;

    uint8_t* ast_arena_free_region;
    const size_t available_arena_bytes;
    size_t* used_arena_bytes;

    std::vector<std::tuple<size_t, size_t, size_t>>*
        aux_code_block_stmt_directory;
    const size_t available_stmt_dir_entries;
    const size_t next_free_stmt_dir_entry;
    size_t* used_stmt_dir_entries;

    explicit
    Parser( std::unordered_map<std::string, Symbol>* sym_table,
            std::vector<Code_Block>* ACBD,
            std::vector<size_t>* acbd_target_entries,
            std::vector<Token>* tok_array,
            uint8_t* arena_region,
            const size_t avail_arena_bytes,
            size_t* nr_used_arena_bytes,
            std::vector<std::tuple<size_t, size_t, size_t>>* stmt_dir,
            const size_t avail_stmt_dir_entries,
            const size_t free_stmt_dir_entry,
            size_t* utilized_stmt_dir_entries)
    : Symbol_Table(sym_table),
      aux_code_block_directory(ACBD),
      which_blocks_to_parse(acbd_target_entries),
      Tokens(tok_array),
      ast_arena_free_region(arena_region),
      available_arena_bytes(avail_arena_bytes),
      used_arena_bytes(nr_used_arena_bytes),
      aux_code_block_stmt_directory(stmt_dir),
      available_stmt_dir_entries(avail_stmt_dir_entries),
      next_free_stmt_dir_entry(free_stmt_dir_entry),
      used_stmt_dir_entries(utilized_stmt_dir_entries)
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

    Parser my_parser = Parser(&Symbol_Table, &code_block_directory,
                              &parsing_quota, &Tokens,
                              ast_arena, ast_arena_size,
                              &ast_arena_used_bytes, &statement_directory,
                              statement_directory_size,
                              statement_directory_next_free_entry,
                              &statement_directory_used_entries);

    ret = my_parser.parse_blocks();

    statement_directory_next_free_entry += statement_directory_used_entries;
    ast_arena_next_free_region_offset   += ast_arena_used_bytes;

    if(ret) [[unlikely]]
    {
        std::cout << "\nParsing FAILED!\n";
        return 1;
    }
    else
        std::cout << "\nParsing was a SUCCESS!\n";

    std::cout << "AST mem arena used bytes  : " << ast_arena_used_bytes << "\n";
    std::cout << "Statement dir used entries: "
              << statement_directory_used_entries << "\n";
    std::cout << "NEW next statement dir free entry: "
              << statement_directory_next_free_entry << "\n";
    std::cout << "NEW next AST Arena free region offset: "
              << ast_arena_next_free_region_offset << "\n";
    std::cout << "\n\n";

    return 0;
}

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
    std::cout << "Token: " << (*Tokens)[cursor].token_value << "\n";
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
    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(Tokens, cursor, 1)

    if((*Tokens)[cursor + 1].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        ++cursor;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }

        lhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    else if((*Tokens)[cursor + 1].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                         std::string((*Tokens)[cursor + 1].token_value),
                                         SYMBOL_KIND_UINT64, 0, symbol_ptr)

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
    else if((*Tokens)[cursor + 1].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset)) %
         alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        lhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
                (std::stoull(std::string((*Tokens)[cursor + 1].token_value)));

        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the two we processed, to the BinOp sign. */
        cursor += 2;
    }

    /* PART II. */

    /* Process the sign, add it to this BinOp object and move cursor by one. */
    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(Tokens, cursor, 1)
    if((*Tokens)[cursor].token_type_ix != TOKEN_TYPE_OPERATOR)
    {
        std::cout << "\nSyntax error: Sign of binary operation is invalid.\n"
                  << "Line: " << (*Tokens)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    std::cout << "Adding BinOp sign: " << (*Tokens)[cursor].token_value << "\n";
    bin_operator = std::string((*Tokens)[cursor].token_value);
    ++cursor;

    /* PART III. */

    /* Exactly the same code as PART I. Parsing an Expression. But PART III
     * also checks for the closing parenthesis of THIS binary operation.
     * TODO: Factor the repeating code out somewhere.
     */

    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(Tokens, cursor, 1)

    if((*Tokens)[cursor].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }

        rhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    else if((*Tokens)[cursor].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                         std::string((*Tokens)[cursor].token_value),
                                         SYMBOL_KIND_UINT64, 0, symbol_ptr)

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
    else if((*Tokens)[cursor].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while
        (((uintptr_t)(ast_arena_region_ptr + wr_offset)) %
         alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
                (std::stoull(std::string((*Tokens)[cursor].token_value)));

        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the token we processed, to the close paren. */
        cursor += 1;
    }

    /* Last part of PART III: Check the closing paren. Don't check semicolon. */

    if((*Tokens)[cursor].token_type_ix != TOKEN_TYPE_CLOSE_PAREN)
    [[unlikely]]
    {
        std::cout << "\nSyntax error: Missing closing parenthesis.\n"
                  << "Line: " << (*Tokens)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    ++cursor;

    /* At the end of EACH Node-constructing token processor function, once all
     * the parts of its Node object are available, it returns to the reserved
     * space of sizeof(that_Node) (which is given by the untouched bytes_used
     * pointer handed down to the function from whoever called it), that it
     * reserved at the START of the function and constructs its object there.
     */
    auto wn_node_ptr = new (ast_arena_region_ptr + (*new_node_wr_offset))
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
    size_t wr_offset = *bytes_used;
    size_t next_node_wr_offset;
    uint8_t ret;

    /* A pointer to a Symbol object is needed as the LHS data member.
     * If present in the Symbol Table, add a pointer to it. If not,
     * construct it in the Symbol Table and then add the pointer to it.
     */
    ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                     std::string((*Tokens)[cursor].token_value),
                                     SYMBOL_KIND_UINT64, 0, lhs_symbol_ptr)

    /* Now parse the RHS of the assignment. Parser expects the initial memory
     * address in the pointer it gave us (to the AST Arena) to point to this
     * Statement's AST Node, so construct it before bumping the given Arena ptr.
     *
     * Actually, Parser will have to NOT expect that, since object alignment
     * requirements might have us bump the pointer with empty space anyway.
     *
     * Parser will have to note down the address of the ptr it gave us,
     * Statement processor returns that same ptr to start of Statement object
     * (with any alignment), plus how many bytes were used beyond the FIRST
     * starting address. So Parser can then put that address we gave it inside
     * an entry in the Auxilliary Code Block Statement Directory.
     */

    /* Construct an AST Assignment Node object. LHS (symbol pointer) filled.
     * RHS (Expression) starting out as a NULL pointer for now. After we see
     * what the RHS looks like, called the proper processor functions to emit
     * the AST Node(s) for it, we will get back a pointer to this Expression
     * AST Node object so we can complete the initialization of the Statement
     * AST Node object with it.
     */

    /* Normally, objects must be aligned to the largest member's size. */

    /* Reserve the space here for an assignment statement AST Node. */

    while(((uintptr_t)(ast_arena_region_ptr + wr_offset))
     % alignof(AST_Node_Statement_Assignment))
        ++wr_offset;
    if(wr_offset > bytes_available) [[unlikely]] { return 1; }

    *this_node_wr_offset = wr_offset;

    wr_offset += sizeof(AST_Node_Statement_Assignment);
    if(wr_offset > bytes_available) [[unlikely]] { return 1; }

    /* Syntax case 1, RHS Node is this object: AST_Node_Expr_UINT64_Literal. */

    VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(Tokens, cursor, 2)

    if((*Tokens)[cursor + 2].token_type_ix == TOKEN_TYPE_NUM_LITERAL_UINT)
    {
        /* Align if needed. */
        while(((uintptr_t)(ast_arena_region_ptr + wr_offset)) % alignof(AST_Node_Expr_UINT64_Literal))
            ++wr_offset;

        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr + wr_offset)
            AST_Node_Expr_UINT64_Literal
               (std::stoull(std::string((*Tokens)[cursor + 2].token_value)));
        wr_offset += sizeof(AST_Node_Expr_UINT64_Literal);
        if(wr_offset > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the three we just processed. */
        cursor += 3;
    }
    /* Syntax case 2, RHS Node is this object: AST_Node_Expr_Identifier. */
    else if((*Tokens)[cursor + 2].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                         std::string((*Tokens)[cursor + 2].token_value),
                                         SYMBOL_KIND_UINT64, 0, symbol_ptr)
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
    else if((*Tokens)[cursor + 2].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        cursor += 2;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr, bytes_available,
                                &wr_offset, &next_node_wr_offset);
        if(ret) { return 1; }
        rhs_expr_node_ptr =
            (AST_Node_Expression*)(ast_arena_region_ptr + next_node_wr_offset);
    }

    /* Semicolon check is last, independent of assignment syntax type. */
    if((*Tokens)[cursor].token_type_ix != TOKEN_TYPE_SEMICOLON)
    [[unlikely]]
    {
        std::cout
             << "\n\nSyntax error: Missing semicolon at end of assignment.\n"
             << "Line: " << (*Tokens)[cursor].token_line_in_src << "\n\n";
        std::abort();
    }
    ++cursor;

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
 * The Parsing Orchestrator makes the AST of a correctly written Hirola program.
 * It also generates the Symbol Table for the program and the Auxilliary Code
 * Block Statement Directory serving as a contiguous array containing the root
 * AST Node of each Code Block in the written source code, containing indices
 * to each statement's AST Node in the AST Arena, completing the full AST.
 *
 * Argument 1: The kind of statement being processed. Dispatches to the
 *             respective statement kind's processor function, which will lead
 *             to (recursive) calls to other constructs' pocessors functions.
 *
 * Argument 2: A pointer to an available section of the memory arena holding all
 *             AST Nodes of the constructed tree contiguously with a cache
 *             locality-friendly topology, given by the Parsing Orchestrator
 *             and handed down to the Parser that calls this function.
 *
 * Argument 3: How many bytes of Arena memory this statement's Nodes can use.
 *
 * Argument 4: Pointer to a bookkeeping counter keeping track of how many bytes
 *             the recursively called child nodes' processor functions use up.
 *             That way, each level of this statement's subtree knows where to
 *             write its own AST Node in the arena chunk handed to the function.
 *
 * Argument 5: Index into Auxilliary Code Block Directory to fill out this
 *             Code Block's type and metainformation like function signature.
 *
 * Returns:
 * -------
 *      - 0: OK: Statement, with everything that makes it up, has been parsed.
 *               All of its AST Nodes successfully fit and placed in the given
 *               chunk of the memory arena handed to the function.
 *
 *      - 1: ERR: Not enough memory to store all of the statement's AST Nodes.
 *                In that case, the Parser will alert the Parsing Orchestrator
 *                of this error and wait for a pointer to a larger chunk of
 *                Arena memory to be given to it, so it can attempt to process
 *                the same statement again.
 *
 * NOTE: The initial parsing of the Token array will produce a Symbol Table.
 *       It only constructs the AST representing one to one the given source
 *       code. Only after this, semantic analysis begins, dealing with the
 *       Symbol Table and any present syntax/semantic errors.
 *
 * NOTE: The parser doesn't need to pass a pointer to the Auxilliary Code Block
 *       Statement Directory because it already knows the Arena location of
 *       this statement, because it's just the pointer passed here. Parser
 *       fills out one slot in the Statement Directory for each call here.
 */
uint8_t Parser::parse_statement
                       (size_t* token_cursor,     uint8_t* ast_arena_region_ptr,
                        size_t  bytes_available,  size_t*  bytes_used,
                        size_t  codeblock_dir_ix, bool*    last_statement_seen,
                        size_t* statement_wr_offset_after_alignment,
                        bool* statement_dir_entry_adding)
{

    /* When we're finished parsing this statement, the function lets the Parser
     * know where the last token of this statement was through this pointer.
     */
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
    switch((*Tokens)[cursor].token_type_ix)
    {
    case TOKEN_TYPE_KEYWORD:
    {
        /* Which keyword? */
        if( (*Tokens)[cursor].token_value
                == reserved_keyword_strings[KEYWORD_BLOCK_START] )
        {
            ++cursor;
            /* Which code block type are we starting? */
            if( (*Tokens)[cursor].token_value
                    == reserved_keyword_strings[KEYWORD_PROGRAM] )
            {
                (*aux_code_block_directory)[codeblock_dir_ix]
                    .code_block_type_index = CODE_BLOCK_TYPE_PROGRAM;
                ++cursor;
            }
            else
            {
                std::cout
                     << "\n\n"
                     << "Syntax error: Starting an invalid Code Block type.\n"
                     << "Line: " << (*Tokens)[cursor].token_line_in_src
                     << "\n\n";
                std::abort();
            }
        }
        else if( (*Tokens)[cursor].token_value
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
                 << "Line: " << (*Tokens)[cursor].token_line_in_src
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
        VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(Tokens, cursor, 4)
        /* Check for the equals sign. */
        if((*Tokens)[cursor + 1].token_value != "=")
        [[unlikely]]
        {
            std::cout << "\n\n"
                      << "Syntax error: Assignment started but no '=' found.\n"
                      << "Line: " << (*Tokens)[cursor + 1].token_line_in_src
                      << "\n\n";
            std::abort();
        }
        /* Check for what comes after the equals sign. */
        if(  (*Tokens)[cursor + 2].token_type_ix != TOKEN_TYPE_IDENTIFIER
          && (*Tokens)[cursor + 2].token_type_ix != TOKEN_TYPE_NUM_LITERAL_UINT
          && (*Tokens)[cursor + 2].token_type_ix != TOKEN_TYPE_OPEN_PAREN)
        [[unlikely]]
        {
            std::cout
                 << "\n\n"
                 << "Syntax error: You have something wrong after the equals\n"
                 << "              sign in an assignment. Only 3 things are\n"
                 << "              allowed after the =, which are: Literal,\n"
                 << "              Identifier or '(' for binary operations.\n"
                 << "Line: " << (*Tokens)[cursor + 2].token_line_in_src
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
                                 size_t block_dir_ix)
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

/* NEED TO USE THESE HERE TO BUILD THE 2nd auxilliary table of stmt offsets.
 *
 *    std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>*
        aux_code_block_stmt_directory;
    const size_t available_stmt_dir_entries;
    const size_t next_free_stmt_dir_entry;
    size_t* used_stmt_dir_entries;
 */
        /* TODO: IMPORTANT: For this to work (filling the entry at vector[i])
         *                  we have to make sure the capacity AND the .size()
         *                  are reserved. .reserve(N), according to Claude,
         *                  only sets the vector's capacity to N elements,
         *                  but the .size() is still zero (no default-inited
         *                  entries have been spawned into existence).
         *                  To reserve memory for N slots to avoid too many
         *                  hidden heap allocations AND to actually create
         *                  the elements default-initialized so i can write
         *                  to element N of the vector atbitrarily without
         *                  having to push/emplace_back, we use the vector
         *                  initialization syntax:
         *
         *                  //size AND capacity initialized to N:
         *                  std::vector<T> my_vec(N);
         *
         *
         */

        if(ret) [[unlikely]] { return 1; }

        if(statement_dir_entry_adding){
            (*aux_code_block_stmt_directory)
            [next_free_stmt_dir_entry + (*used_stmt_dir_entries)]
                = std::make_tuple
                    (block_dir_ix, statement_ix++, arena_offset_to_statement);

            ++(*used_stmt_dir_entries);

            if( (*used_stmt_dir_entries) >= available_stmt_dir_entries )
            [[unlikely]]
            { return 1; }
        }

    }
    return 0;
}

uint8_t Parser::parse_blocks(){
    uint8_t ret;
    size_t start_cursor;

    for(size_t i = 0; i < which_blocks_to_parse->size(); ++i)
    {
        start_cursor = (*aux_code_block_directory)
                            [(*which_blocks_to_parse)[i]]
                                .start_token_index;

        ret = parse_statements(&start_cursor, (*which_blocks_to_parse)[i]);

        if(ret)
        [[unlikely]]
        {
            std::cout << "[ERR] parse_statements() returned 1. Out of mem.\n\n";
            std::abort();
        }
    }

    std::cout << "\n[OK] Parsing successful!\n\n";

    return 0;
}
