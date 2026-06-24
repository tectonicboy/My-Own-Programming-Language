#include <cstdint>

/* This header file defines the different kinds of AST Node classes based on
 * the language's grammar, which is given here too. It also defines Symbol Table
 * resources like named indices into a lookup table of Symbol kinds, much like
 * the Token header defines named indices into a lookup table of Token kinds.
 *
 * Language Grammar:
 * ----------------
 *
 * Program ::= Statement+ "PROG_END"
 *
 * Statement ::= AssignmentStatement   (later: FuncCallStatement, IF ELSE, etc)
 *
 * AssignmentStatement ::= IDENTIFIER "=" Expression ";"
 *
 * Expression ::= INT_LITERAL | IDENTIFIER | BinaryOperation
 *
 * BinaryOperation ::= "(" Expression BinaryOperator Expression ")"
 *
 * BinaryOperator ::= "+" | "-" | "*" | "/"
 *
 * IDENTIFIER ::= [a-zA-Z_]+
 *
 * INT_LITERAL ::= [0-9]+
 *
 * Semantic Rules:
 * --------------
 *
 * Rule 1: Each complex expression must be parenthesized. This gives the order
 *         that operations are carried out in, no ambiguous operator precedence.
 *
 * Rule 2: Variables must be assigned to, before being read. No declarations
 *         without initialization are possible, for clarity and unambiguity.
 *
 * Rule 3: "PROG_END" signifies the end of the program. Place at source end.
 *
 * Rule 4: Each Statement ends with a semicolon.
 *
 * Rule 5: Unsigned 64-bit integer-only expressions & expression operands.
 *         Strings, negative and floating point numbers don't exist yet.
 *         Functions and control flow don't exist yet. Pointers don't exist yet.
 *         OS API calls (syscalls) don't exist yet.
 */

/* Lookup tables with named indices for the kinds of Statements, Expressions
 * and Symbols found in the language's definition.
 */

constexpr uint8_t TOTAL_SYMBOL_KINDS     = 2;
constexpr uint8_t TOTAL_STATEMENT_KINDS  = 5;
constexpr uint8_t TOTAL_EXPRESSION_KINDS = 3;

constexpr uint8_t SYMBOL_KIND_UINT64   = 0;
constexpr uint8_t SYMBOL_KIND_FUNCTION = 1;

constexpr uint8_t STATEMENT_KIND_ASSIGNMENT = 0;
constexpr uint8_t STATEMENT_KIND_IF         = 1;
constexpr uint8_t STATEMENT_KIND_ELSE       = 2;
constexpr uint8_t STATEMENT_KIND_UNTIL_LOOP = 3;
constexpr uint8_t STATEMENT_KIND_FUNC_CALL  = 4;

constexpr uint8_t EXPR_KIND_UINT64_LITERAL = 0;
constexpr uint8_t EXPR_KIND_IDENTIFIER     = 1;
constexpr uint8_t EXPR_KIND_BIN_OPERATION  = 2;

constexpr std::array<const char*, TOTAL_SYMBOL_KINDS>
symbol_kinds_lookuptable =
{
    "uint64",
    "function"
};

constexpr std::array<const char*, TOTAL_STATEMENT_KINDS>
statement_kinds_lookuptable =
{
    "assignment",
    "if_statement",
    "else_statement",
    "until_loop",
    "function_call"
};

constexpr std::array<const char*, TOTAL_EXPRESSION_KINDS>
expression_kinds_lookuptable =
{
    "uint64_literal",
    "identifier",
    "binary_operation"
};

/*----------------------------------------------------------------------------*/

/* The Symbol class. */
class Symbol
{
public:

    /* Every symbol in the source code has a name, a type and a value. */
    std::string symbol_name;
    uint8_t symbol_kind_ix;

    /* For now, we only have u64 variables. When we add functions, make this
     * a union, and the 64 bits are split into several-bit sections that
     * fully describe the function signature: return type, number of arguments,
     * the type of each agument. For example, if there are 4 possible types
     * in the language, two bits are enough to store the information about
     * the return type and each argument's type. Similarly, if the maximum
     * number of arguments allowed by the language is 8, then 3 bits are enough
     * to store the count of arguments the function expects when called.
     * These bitfield subsections will be documented.
     */
    uint64_t value;

    /* Constructor. */
    explicit Symbol(uint8_t kind_input, std::string name_input, uint64_t val_in)
    : symbol_name(name_input), symbol_kind_ix(kind_input), value(val_in) {}

    void print_symbol_name(void) const
    {
        std::cout << symbol_name;
    }

    void print_symbol_kind(void) const
    {
        std::cout << symbol_kinds_lookuptable[symbol_kind_ix];
    }

    void print_symbol_value(void) const
    {
        std::cout << value;
    }
};

/*----------------------------------------------------------------------------*/

/* Abstract class. Concrete subclasses are the different expression kinds. */
class AST_Node_Expression
{
public:

    uint8_t expr_kind_ix;

protected:

    AST_Node_Expression(uint8_t kind_input) : expr_kind_ix(kind_input) {}

public:

    virtual ~AST_Node_Expression () = default;

    void print_expr_kind(void) const
    {
        std::cout << expression_kinds_lookuptable[expr_kind_ix];
    }

    virtual void print_node(void) const = 0;
};

/*----------------------------------------------------------------------------*/

/* The concrete subclass representing an expression of type Binary Operation. */

class AST_Node_Expr_BinOp : public AST_Node_Expression
{
public:

    AST_Node_Expression* lhs_expression;
    AST_Node_Expression* rhs_expression;

    /* SSO: Small String Optimization - used by the STL and other libraries to
     *      not make a heap allocation for small strings. Instead, the buffer
     *      itself where the characters are stored will be inside the object
     *      rather than in a heap-allocated buffer. This is a performance win.
     */
    std::string binary_operator;

    /* Constructor. */
    explicit AST_Node_Expr_BinOp
        (AST_Node_Expression* lhs_input,
         AST_Node_Expression* rhs_input,
         std::string operator_input)
    : AST_Node_Expression(EXPR_KIND_BIN_OPERATION),
      lhs_expression(lhs_input),
      rhs_expression(rhs_input),
      binary_operator(operator_input) {}

    /* Default compiler-generated destructor. */
    ~AST_Node_Expr_BinOp () = default;

    void print_node(void) const override
    {
        std::cout << "(";
        lhs_expression.print_node();
        std::cout << " " << binary_operator << " ";
        rhs_expression.print_node();
        std::cout << ")";
    }
};

/*----------------------------------------------------------------------------*/

/* The concrete subclass representing an expression of type UINT64 Literal. */

class AST_Node_Expr_UINT64_Literal : public AST_Node_Expression
{
public:

    uint64_t value;

    /* Constructor. */
    explicit AST_Node_Expr_UINT64_Literal(uint64_t n)
    : AST_Node_Expression(EXPR_KIND_UINT64_LITERAL), value(n) {}

    /* Destructor. */
    ~AST_Node_Expr_UINT64_Literal () = default;

    void print_node(void) const override
    {
        std::cout << value;
    }
};

/*----------------------------------------------------------------------------*/

/* The concrete subclass representing an expression of kind Identifier. */

class AST_Node_Expr_Identifier : public AST_Node_Expression
{
public:

    Symbol* symbol;

    /* Constructor. */
    explicit AST_Node_Expr_Identifier (Symbol* s)
    : AST_Node_Expression(EXPR_KIND_IDENTIFIER), symbol(s) {}

    /* Destructor. */
    ~AST_Node_Expr_Identifier () = default;

    void print_node(void) const override
    {
        symbol.print_symbol_name();
    }
};

/*----------------------------------------------------------------------------*/

/* Abstract base class for Statement AST Nodes.
 * Concrete subclasses represent the different statement kinds.
 */
class AST_Node_Statement
{
public:

    uint8_t statement_kind_ix;

protected:

    AST_Node_Statement(uint8_t kind_input) : statement_kind_ix(kind_input) {}

public:

    /* Default, compiler-generated destructor. */
    virtual ~AST_Node_Statement () = default;

    void print_statement_kind(void) const
    {
        std::cout << statement_kinds_lookuptable[statement_kind_ix];
    }

    virtual void print_node(void) const = 0;
};

/*----------------------------------------------------------------------------*/

/* The concrete Statement subclass representing an assignment statement. */

class AST_Node_Statement_Assignment : public AST_Node_Statement
{
public:

    Symbol*              lhs_identifier;
    AST_Node_Expression* rhs_expression;

    /* Constructor. */
    explicit AST_Node_Statement_Assignment
        (Symbol* lhs_name_input, AST_Node_Expression* rhs_expr_input)
    : AST_Node_Statement(STATEMENT_KIND_ASSIGNMENT),
      lhs_identifier(lhs_name_input), rhs_expression(rhs_expr_input) {}

    ~AST_Node_Statement_Assignment () = default;

    void print_node(void) const override {
        lhs_identifier.print_symbol_name();
        std::cout << " = ";
        rhs_expression.print_node();
    }
};

/*----------------------------------------------------------------------------*/

/* Functions on how to process each kind of code construct. */

#define ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(symbols, iter, name, type, val, ptr) \
    (iter) = (symbols).find((name));                                          \
    if( (iter) == (symbols).end() )                                           \
    [[unlikely]]                                                              \
    {                                                                         \
        (iter) =                                                              \
            ((symbols).emplace((name), Symbol((type), (name), (val)))).first; \
    }                                                                         \
    (ptr) = &((iter)->second);


uint8_t parse_bin_op_expr(size_t*      token_cursor,
                          uint8_t*     ast_arena_region_ptr,
                          const size_t bytes_available
                          size_t*      bytes_used)
{
    Symbol* symbol_ptr;
    AST_Node_Expression* rhs_expr_node_ptr;
    AST_Node_Expression* lhs_expr_node_ptr;
    std::string bin_operator;
    size_t cursor = *token_cursor;
    uint8_t ret;

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
    if(Tokens[cursor + 1].token_type_ix == TOKEN_TYPE_OPEN_PAREN)
    {
        ++cursor;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr,
                                bytes_available, bytes_used);
        if(ret) { return 1; }
    }






    *token_cursor = cursor;
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
uint8_t parse_assignment_statement(size_t*      token_cursor,
                                   uint8_t*     ast_arena_region_ptr,
                                   const size_t bytes_available
                                   size_t*      bytes_used)
{
    std::unordered_map<std::string, Symbol>::iterator symbol_table_iterator;
    Symbol* symbol_ptr;
    AST_Node_Expression* rhs_expr_node_ptr;
    AST_Node_Statement_Assignment* statement_node_ptr;
    size_t cursor = *token_cursor;
    uint8_t ret;

    /* A pointer to a Symbol object is needed as the LHS data member.
     * If present in the Symbol Table, add a pointer to it. If not,
     * construct it in the Symbol Table and then add the pointer to it.
     */
    ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                     Tokens[cursor].token_value,
                                     SYMBOL_KIND_UINT64, 0, symbol_ptr)

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
    while(ast_arena_region_ptr % alignof(AST_Node_Statement_Assignment))
    {
        ++ast_arena_region_ptr;
        ++(*bytes_used);
    }
    if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

    statement_node_ptr = new (ast_arena_region_ptr)
       AST_Node_Statement_Assignment(symbol_ptr, nullptr);

    ast_arena_region_ptr += sizeof(AST_Node_Statement_Assignment);
    *bytes_used          += sizeof(AST_Node_Statement_Assignment);
    if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

    /* Syntax case 1, RHS Node is this object: AST_Node_Expr_UINT64_Literal. */
    if(Tokens[cursor + 2].token_type_ix == TOKEN_TYPE_LITERAL_UINT)
    {
        /* Align if needed. */
        while(ast_arena_region_ptr % alignof(AST_Node_Expr_UINT64_Literal))
        {
            ++ast_arena_region_ptr;
            ++(*bytes_used);
        }
        if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr)
            AST_Node_Expr_UINT64_Literal
               (std::stoull(std::string(Tokens[cursor + 2].token_value)));

        statement_node_ptr->rhs_expression = rhs_expr_node_ptr;

        ast_arena_region_ptr += sizeof(AST_Node_Expr_UINT64_Literal);
        *bytes_used          += sizeof(AST_Node_Expr_UINT64_Literal);
        if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the three we just processed. */
        cursor += 3;
    }
    /* Syntax case 2, RHS Node is this object: AST_Node_Expr_Identifier. */
    else if(Tokens[cursor + 2].token_type_ix == TOKEN_TYPE_IDENTIFIER)
    {
        ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(Symbol_Table, symbol_table_iterator,
                                         Tokens[cursor + 2].token_value,
                                         SYMBOL_KIND_UINT64, 0, symbol_ptr)
        /* Align if needed. */
        while(ast_arena_region_ptr % alignof(AST_Node_Expr_Identifier))
        {
            ++ast_arena_region_ptr;
            ++(*bytes_used);
        }
        if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

        rhs_expr_node_ptr = new (ast_arena_region_ptr)
            AST_Node_Expr_Identifier(symbol_ptr);

        statement_node_ptr->rhs_expression = rhs_expr_node_ptr;

        ast_arena_region_ptr += sizeof(AST_Node_Expr_Identifier);
        *bytes_used          += sizeof(AST_Node_Expr_Identifier);
        if(*bytes_used > bytes_available) [[unlikely]] { return 1; }

        /* Move token cursor past the three we processed. */
        cursor += 3;
    }
    /* Syntax case 3. */
    else if(Tokens[cursor + 2].token_type_ix == OPEN_PAREN)
    {
        cursor += 2;
        ret = parse_bin_op_expr(&cursor, ast_arena_region_ptr,
                                bytes_available, bytes_used);
        if(ret) { return 1; }
    }

    /* Semicolon check is last, independent of assignment syntax type. */
    if(Tokens[cursor].token_type_ix != TOKEN_TYPE_SEMICOLON)
    {
        cout << "\n\n"
             << "Syntax error: Missing semicolon at end of assignment.\n"
             << "Line: " << Tokens[cursor].token_line_in_src
             << "\n\n";
        std::abort();
    }
    ++cursor;

    /* Update the Token cursor for upstream calls. */
    *token_cursor = cursor;

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

uint8_t parse_statement(size_t* token_arr_ix,     uint8_t* ast_arena_region_ptr,
                        size_t  bytes_available,  size_t*  bytes_used,
                        size_t  codeblock_dir_ix, bool*    last_statement_seen)
{

    /* When we're finished parsing this statement, the function lets the Parser
     * know where the last token of this statement was through this pointer.
     */
    size_t token_cursor = *token_arr_ix;

    /* The grammar is simple enough that the first token of the statement
     * reveals exactly what type of statement it is, depending on the Token type
     * so we can call the respective Statement Processor Function here. For
     * statements that don't add an AST Node, we parse them here without having
     * a special processor function for them, for example BLOCK_START.
     */

    /* Which token type? */
    switch(Tokens[token_cursor].token_type_ix)
    {
    case TOKEN_TYPE_KEYWORD:
    {
        /* Which keyword? */
        switch(Tokens[token_cursor].token_value)
        {
        case reserved_keyword_strings[KEYWORD_BLOCK_START]:
        {
            ++token_cursor;
            /* Which code block type are we starting? */
            switch(Tokens[token_cursor].token_value)
            {
            case reserved_keyword_strings[KEYWORD_PROGRAM]:
            {
                aux_code_block_directory[codeblock_dir_ix]
                    .code_block_type_index = CODE_BLOCK_TYPE_PROGRAM;
                ++token_cursor;
                break;
            }
            default:
            {
                cout << "\n\n"
                     << "Syntax error: Starting an invalid Code Block type.\n"
                     << "Line: " << Tokens[token_cursor].token_line_in_src
                     << "\n\n";
                std::abort();
            }
            } /* 2nd inner switch end. */
            break;
        }
        case reserved_keyword_strings[KEYWORD_BLOCK_END]:
        {
            ++token_cursor;
            *last_statement = true;
            break;
        }
        default:
        {
            cout << "\n\n"
                 << "Unexpected error: Invalid keyword.\n"
                 << "Line: " << Tokens[token_cursor].token_line_in_src
                 << "\nThis should never happen. Must be investigated.\n\n";
            std::abort();
        }
        } /* inner switch end. */
        break;
    }
    /* This starts an assignment statement. */
    case TOKEN_TYPE_IDENTIFIER:
    {
        /* Quick syntax checks then call the processor function. */

        /* At least 5 more tokens are needed for a minimal assignment:
         * identifier = identifier ; BLOCK_END
         * If the token array ends before that, the program is incomplete.
         */
        if( ! (Tokens.size() - token_cursor > 4) )
        [[unlikely]]
        {
            cout << "\n\n"
                 << "Syntax error: Incomplete assignment statement seen.\n"
                 << "Line: " << Tokens[token_cursor].token_line_in_src
                 << "\n\n";
            std::abort();
        }
        /* Check for the equals sign. */
        if(Tokens[token_cursor + 1].token_value != "=")
        [[unlikely]]
        {
            cout << "\n\n"
                 << "Syntax error: Assignment started but no '=' found.\n"
                 << "Line: " << Tokens[token_cursor + 1].token_line_in_src
                 << "\n\n";
            std::abort();
        }
        /* Check for what comes after the equals sign. */
        if(    Tokens[token_cursor + 2].token_type_ix != TOKEN_TYPE_IDENTIFIER
            && Tokens[token_cursor + 2].token_type_ix != TOKEN_TYPE_LITERAL_UINT
            && Tokens[token_cursor + 2].token_type_ix != TOKEN_TYPE_OPEN_PAREN)
        [[unlikely]]
        {
            cout << "\n\n"
                 << "Syntax error: You have something wrong after the equals\n"
                 << "              sign in an assignment. Only 3 things are\n"
                 << "              allowed after the =, which are: Literal,\n"
                 << "              Identifier or '(' for binary operations.\n"
                 << "Line: " << Tokens[token_cursor + 2].token_line_in_src
                 << "\n\n";
            std::abort();
        }

        /* Start of assignment statement looks good. Parse it. */
        parse_assignment_statement(token_cursor, ast_arena_region_ptr,
                                   bytes_available, bytes_used);
        break;
    }
    } /* outer switch end. */



    *token_arr_ix = token_cursor;

}
