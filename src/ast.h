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

/* Abstract base class for used symbols in the program source code.
 *
 * Variable & function names are exampes of symbols - concrete subclasses.
 */
class Symbol
{
public:

    /* Every symbol in the source code has a name and a kind. */
    std::string symbol_name;
    uint8_t symbol_kind_ix;

protected:

    /* Constructor of abstract base class is used to initialize the members that
     * are common to all concrete subclasses.
     *
     * It is called in the concrete subclasses' constructor initializer list.
     * Whoever instantiates objects only calls the derived constructors, which
     * is made to take the arguments of this abstract base constructor AND
     * its own arguments after that.
     *
     * This is neat, because we factor out commonality into one place.
     */
    explicit Symbol(uint8_t kind_input, std::string name_input)
    : symbol_name(name_input), symbol_kind_ix(kind_input) {}

public:

    /* Virtual destructor. */
    virtual ~Symbol () = default;

    void print_symbol_name(void) const
    {
        std::cout << symbol_name;
    }

    void print_symbol_kind(void) const
    {
        std::cout << symbol_kinds_lookuptable[symbol_kind_ix];
    }

    virtual void print_symbol_contents(void) const = 0;
};

/*----------------------------------------------------------------------------*/

/* The concrete sublcass representing a symbol of type UINT64. */

class Symbol_Type_UINT64 : public Symbol
{
public:

    uint64_t symbol_value;

    explicit Symbol_Type_UINT64(uint64_t val_input, std::string name_input)
    : Symbol(SYMBOL_KIND_UINT64, name_input),
      symbol_value(val_input) {}

    ~Symbol_Type_UINT64 () = default;

    void print_symbol_contents(void) const override
    {
        std::cout << symbol_value;
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

    AST_Node_Expression& lhs_expression;
    AST_Node_Expression& rhs_expression;

    /* SSO: Small String Optimization - used by the STL and other libraries to
     *      not make a heap allocation for small strings. Instead, the buffer
     *      itself where the characters are stored will be inside the object
     *      rather than in a heap-allocated buffer. This is a performance win.
     */
    std::string binary_operator;

    /* Constructor. */
    explicit AST_Node_Expr_BinOp
        (AST_Node_Expression& lhs_input,
         AST_Node_Expression& rhs_input,
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

    Symbol& symbol;

    /* Constructor. */
    explicit AST_Node_Expr_Identifier (Symbol& s)
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

    Symbol&              lhs_identifier;
    AST_Node_Expression& rhs_expression;

    /* Constructor. */
    explicit AST_Node_Statement_Assignment
        (Symbol& lhs_name_input, AST_Node_Expression& rhs_expr_input)
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

/* Functions on how to process each kind of Statement when the current token
 * read by the parser indicates we've hit the start of such a statement.
 */

/* Top-level statement processor.
 *
 * This is the only function called by the Parsing Orchestrator, whose job is to
 * maintain the cache locality-friendly memory arena used to store all Nodes of
 * the constructed AST, along with any necessary bookkeeping information. The
 * Parsing Orchestrator makes the AST of a correctly written Hirola program. It
 * also generates the Symbol Table for the program.
 *
 * Argument 1: The kind of statement being processed. Dispatches to the
 *             respective statement kind's processor function depending on what
 *             type of statement it's asked to parse.
 *
 * Argument 2: A pointer to an available section of the memory arena holding all
 *             AST Nodes of the constructed tree contiguously in a cache
 *             locality-friendly topology, given by the Parsing Orchestrator.
 *
 * Argument 3: How many bytes of memory this statement's Nodes can use.
 *
 * Argument 4: Pointer to a bookkeeping counter keeping track of how many bytes
 *             the recursively called child nodes' processor functions use up.
 *             That way, each level of this statement's subtree knows where to
 *             write its own AST Node in the arena chunk handed to the function.
 *
 * Returns:
 * -------
 *      - 0: OK: Statement, with everything that makes it up, has been parsed.
 *               All of its AST Nodes successfully fit and placed in the given
 *               chunk of the memory arena handed to the function.
 *
 *      - 1: ERR: Not enough memory to store all of the statement's AST Nodes.
 *                In that case, the Parsing Orchestrator will call this function
 *                again for this statement with more memory available to use.
 */

uint8_t parse_statement(uint8_t statement_kind,  uint8_t* nodes_output_mem_ptr,
                        size_t  bytes_available, size_t*  bytes_used)
{
    switch(statement_kind)
    {
    case STATEMENT_KIND_ASSIGNMENT:
    {
        // Grammar: AssignmentStatement ::= IDENTIFIER "=" Expression ";"

        /* Add the LHS identifier to the Symbol Table if not found there. */
        std::unordered_map<std::string, Symbol>::iterator it = symbol_table.find("")

        break;
    } /* case   end. */
    } /* switch end. */





}















