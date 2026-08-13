/* Counters for total symbol kinds, statement kinds and expression kinds. */
constexpr uint8_t TOTAL_SYMBOL_KINDS     = 2;
constexpr uint8_t TOTAL_STATEMENT_KINDS  = 5;
constexpr uint8_t TOTAL_EXPRESSION_KINDS = 3;

/* Lookup tables with named indices for the kinds of Statements, Expressions
 * and Symbols found in the language.
 */
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

class Symbol
{
public:
    std::string symbol_name;
    uint8_t     symbol_kind_ix;
    uint64_t    symbol_type;
    uint64_t    SSA_IR_mangle_counter;

    /* Constructor. */
    explicit
    Symbol(uint8_t kind_input, std::string name_input, uint64_t type_in)
    : symbol_name(name_input), symbol_kind_ix(kind_input), symbol_type(type_in),
      SSA_IR_mangle_counter(1) {}

    void print_symbol_name(void) const
    {
        std::cout << symbol_name;
    }

    void print_symbol_kind(void) const
    {
        std::cout << symbol_kinds_lookuptable[symbol_kind_ix];
    }

    void print_symbol_type(void) const
    {
        std::cout << symbol_type;
    }
};
/*----------------------------------------------------------------------------*/

/* Abstract class. Concrete subclasses are the different expression kinds. */
class AST_Node_Expression
{
public:
    uint8_t expr_kind_ix;

    AST_Node_Expression(uint8_t kind_input) : expr_kind_ix(kind_input) {}

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
    std::string binary_operator;

    /* Constructor. */
    explicit AST_Node_Expr_BinOp
        (AST_Node_Expression* lhs_input, AST_Node_Expression* rhs_input,
         std::string operator_input)
    : AST_Node_Expression(EXPR_KIND_BIN_OPERATION), lhs_expression(lhs_input),
      rhs_expression(rhs_input), binary_operator(operator_input) {}

    void print_node(void) const override
    {
        std::cout << "(";
        lhs_expression->print_node();
        std::cout << " " << binary_operator << " ";
        rhs_expression->print_node();
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

    void print_node(void) const override
    {
        symbol->print_symbol_name();
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

    void print_node(void) const override {
        lhs_identifier->print_symbol_name();
        std::cout << " = ";
        rhs_expression->print_node();
    }
};
/*----------------------------------------------------------------------------*/
