/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Token Array and associated bookkeeping items. */

/* Lookup table of current language reserved keywords. */
constexpr size_t total_keywords = 3;

constexpr uint32_t KEYWORD_BLOCK_START = 0;
constexpr uint32_t KEYWORD_BLOCK_END   = 1;
constexpr uint32_t KEYWORD_PROGRAM     = 2;

constexpr std::array<const char*, total_keywords>
reserved_keyword_strings =
{
    "BLOCK_START",
    "BLOCK_END",
    "PROGRAM"
};

/* Lookup table of current token types accepted in the language. */
constexpr size_t total_token_types = 7;

constexpr uint32_t TOKEN_TYPE_IDENTIFIER       = 0;
constexpr uint32_t TOKEN_TYPE_KEYWORD          = 1;
constexpr uint32_t TOKEN_TYPE_OPEN_PAREN       = 2;
constexpr uint32_t TOKEN_TYPE_CLOSE_PAREN      = 3;
constexpr uint32_t TOKEN_TYPE_OPERATOR         = 4;
constexpr uint32_t TOKEN_TYPE_SEMICOLON        = 5;
constexpr uint32_t TOKEN_TYPE_NUM_LITERAL_UINT = 6;

constexpr std::array<const char*, total_token_types>
token_type_strings =
{
    "Identifier",
    "Keyword",
    "Open Parenthesis",
    "Close Parenthesis",
    "Operator",
    "Semicolon",
    "Number Literal Unsigned Int"
};

/* The descriptor of a Token. Members arranged to eliminate padding bytes. */
class Token
{
public:
    std::string_view  token_value;
    uint64_t          token_line_in_src;
    uint32_t          token_col_in_src;
    uint32_t          token_type_ix;

    /* Constructor. */
    explicit Token(std::string_view value_text, uint64_t line_in_src,
                   uint32_t col_in_src, uint32_t type_ix)
    : token_value(value_text), token_line_in_src(line_in_src),
      token_col_in_src(col_in_src), token_type_ix(type_ix) {}

    void Print_Token_Info(void) const
    {
        std::cout << "---------------------------------\n"
                  << "Token type  : " << token_type_strings[token_type_ix]
                  << "\n"
                  << "Token value : " << token_value << "\n"
                  << "At src line : " << token_line_in_src
                  << ":" << token_col_in_src << "\n"
                  << "---------------------------------\n";
    }
};

/* The Token Array. */

class Token_Array
{
private:
    constexpr static size_t token_array_default_init_capcity = 100'000;
    size_t initial_capacity;
    std::vector<Token> token_array_vec;

public:
    /* Constructor. */
    explicit Token_Array(const size_t init_capacity_in, bool prefill_entries)
    {
        if( ! init_capacity_in )
            initial_capacity = token_array_default_init_capcity;
        else
            initial_capacity = init_capacity_in;

        if(prefill_entries)
            token_array_vec =
                std::vector(initial_capacity, Token("", 0, 0, 0));
        else
                token_array_vec.reserve(initial_capacity);
    }

    /* Move constructor.
     *
     * For example, used to transport it from a Lexer to a Parsing Orchestrator.
     */
    Token_Array(Token_Array&& old_token_arr)
    : initial_capacity(old_token_arr.token_array_vec.capacity()),
      token_array_vec(std::move(old_token_arr.token_array_vec)) {}

    /* Overloaded operator[]. */

    /* Returns a mutable Lvalue reference to an entry, directly modifyable. */
    Token& operator[](const size_t token_ix)
    {
        return token_array_vec[token_ix];
    }

    /* Returns a const Lvalue reference to an entry, not modifyable. */
    const Token& operator[](const size_t token_ix) const
    {
        return token_array_vec[token_ix];
    }

    /* Get size */
    size_t size(void)
    {
        return token_array_vec.size();
    }

    /* Emplace at the back. */
    void emplace_back(std::string_view token_val, uint64_t src_line,
                      uint32_t src_col, uint32_t type)
    {
        token_array_vec.emplace_back(Token(token_val, src_line, src_col, type));
        return;
    }

#define VERIFY_N_TOKENS_AFTER_CURSOR_EXIST(tok_arr, cursor, N) \
    if((tok_arr)->size() - ((cursor) + 1) < (N)) [[unlikely]]  \
    {                                                          \
        std::cout << "Error: Incomplete program.\n";           \
        std::abort();                                          \
    }
};

/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Code Block Directory. */

/* Lookup table of Code Block types. */
constexpr size_t   total_code_block_types  = 1;
constexpr uint32_t CODE_BLOCK_TYPE_PROGRAM = 0;

constexpr
std::array<const char*, total_code_block_types> code_block_type_strings =
{
    "primary program code block"
};

/* Descriptor of a single entry in the Code Block Directory. */
class Code_Block_Directory_Entry
{
public:
    size_t start_token_ix;
    size_t end_token_ix;
    size_t code_block_type_ix;

    /* Constructor. */
    explicit
    Code_Block_Directory_Entry(size_t start_ix, size_t end_ix, size_t type_ix)
    : start_token_ix(start_ix), end_token_ix(end_ix),
    code_block_type_ix(type_ix) {}

    /* Pretty printer. */
    void print_code_block_info(void) const
    {
        std::cout << "\n--------------------------------------------------"
                  << "\nCode Block Type: "
                  << code_block_type_strings[code_block_type_ix]
                  << "\nStart Token Index: " << start_token_ix
                  << "\nEnd   Token Index: " << end_token_ix
                  << "\n--------------------------------------------------\n";
    }
};

/* The Code Block Directory.
 *
 * Used exclusively in the compiler frontend. Spawned into existence by a Lexer,
 * then given to a Parsing Orchestrator via the custom move constructor.
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
     * For now, the Lexer only reserves capacity, size is still 0 at start
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
     * to the Parsing Orchestrator, avoiding wasteful copies.
     */
    Code_Block_Directory(Code_Block_Directory&& old_dir)
    : initial_capacity(old_dir.initial_capacity),
      code_block_dir_vec(std::move(old_dir.code_block_dir_vec)) {}

    /* Overloaded operator[]. Both for getting a const and a mutable entry. */

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
    inline void
    emplace_back(const size_t start_token_ix, const size_t end_token_ix,
                 const size_t code_block_type_ix)
    {
        code_block_dir_vec.emplace_back(Code_Block_Directory_Entry
            (start_token_ix, end_token_ix, code_block_type_ix));
        return;
    }

    inline size_t size(void) const
    {
        return code_block_dir_vec.size();
    }
};

/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Symbol Table. */

constexpr uint8_t TOTAL_SYMBOL_KINDS   = 2;
constexpr uint8_t SYMBOL_KIND_UINT64   = 0;
constexpr uint8_t SYMBOL_KIND_FUNCTION = 1;

constexpr std::array<const char*, TOTAL_SYMBOL_KINDS>
symbol_kinds_lookuptable =
{
    "uint64",
    "function"
};

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

#define SYMTABLE_EXACT_CONTAINER_TYPE std::unordered_map<std::string, Symbol>

class Symbol_Table
{
private:
    constexpr static size_t symtable_default_init_capacity = 10'000;

public:
    size_t initial_capacity;
    SYMTABLE_EXACT_CONTAINER_TYPE symbol_table_hashmap;

    /* Constructor. */
    Symbol_Table(size_t init_capacity_in)
    {
        if( ! init_capacity_in )
            initial_capacity = symtable_default_init_capacity;
        else
            initial_capacity = init_capacity_in;

        symbol_table_hashmap.reserve(initial_capacity);
    }

    /* Move constructor. For completeless, unused for now. */
    Symbol_Table(Symbol_Table&& old_symtable)
    : initial_capacity(old_symtable.symbol_table_hashmap.size()),
      symbol_table_hashmap(std::move(old_symtable.symbol_table_hashmap)) {}

#define ADD_SYMBOL_IF_ABSENT_AND_GET_PTR(symbols, iter, name, type, val, ptr)  \
    (iter) = (symbols)->symbol_table_hashmap.find((name));                     \
    if( (iter) == (symbols)->symbol_table_hashmap.end() ) [[unlikely]]         \
    {                                                                          \
        (iter) = ((symbols)->symbol_table_hashmap.emplace                      \
                        ((name), Symbol((type), (name), (val)))).first;        \
    }                                                                          \
    (ptr) = &((iter)->second);

#define \
EMIT_ERR_IF_SYMBOL_ABSENT_OR_GET_PTR(symbols, iter, name, ptr, src_line)       \
    (iter) = (symbols)->symbol_table_hashmap.find((name));                     \
    if( (iter) == (symbols)->symbol_table_hashmap.end() ) [[unlikely]]         \
    {                                                                          \
        std::cout << "Error: Symbol on RHS of assignment not initialized.\n";  \
        std:: cout << "Line: " << (src_line) << "\n";                          \
        std::abort();                                                          \
    }                                                                          \
    (ptr) = &((iter)->second);

};

/*----------------------------------------------------------------------------*/

/* COMPILER BOOKKEEPING: The Statement Directory. */

/* Descriptor of a single entry in the Statement Directory. */
class Statement_Directory_Entry
{
public:
    size_t code_block_ix;
    size_t statement_ix;
    size_t root_ast_node_arena_offset;

    /* Constructor. */
    explicit Statement_Directory_Entry
       (size_t code_block_ix_in, size_t statement_ix_in, size_t arena_offset_in)
    : code_block_ix(code_block_ix_in), statement_ix(statement_ix_in),
      root_ast_node_arena_offset(arena_offset_in) {}

    /* Pretty printer. */
    void print_statement_info(void) const
    {
        std::cout
        << "-------------------------------------------------------\n"
        << "Code Block Index          : " << code_block_ix << "\n"
        << "Statement  Index          : " << statement_ix  << "\n"
        << "Root AST Node Arena Offset: " << root_ast_node_arena_offset
        << " bytes.\n";
    }
};

/* The Statement Directory.
 *
 * Used first in the compiler frontend spawned by a Parsing Orchestrator. A
 * pointer to it is given to worker Parsers. Then, ownership of it goes to the
 * IR Generation Orchestrator in the compiler middle-end, via move semantics.
 */
class Statement_Directory
{
private:
    constexpr static size_t statement_dir_default_initial_capacity = 10'000;
    size_t initial_capacity;
    std::vector<Statement_Directory_Entry> statement_dir_vec;

public:
    /* Regular constructor.
     *
     * Pass 0 for initial_capacity_in to use default initial capacity.
     *
     * When initializing the std::vector container itself, the caller either
     * tells us to only reserve memory capacity, or to go ahead and preconstruct
     * the entries, so the user can start writing to arbitrary indices.
     * Right now, the Parsing Orchestrator only reserves memory capacity, with
     * .size() still starting out as 0.
     */
    explicit
    Statement_Directory(size_t init_capa_in, bool prefill_default_slots)
    {
        if( ! init_capa_in )
            initial_capacity = statement_dir_default_initial_capacity;
        else
            initial_capacity = init_capa_in;

        if(prefill_default_slots)
            statement_dir_vec = std::vector
                (initial_capacity, Statement_Directory_Entry(0, 0, 0));
        else
            statement_dir_vec.reserve(initial_capacity);
    }

    /* Move constructor.
     *
     * Used when passing ownership of the Statement Directory from the Parsing
     * Orchestrator to the IR Generation Orchestrator, avoiding wasteful copies.
     */
    Statement_Directory(Statement_Directory&& old_dir)
    : initial_capacity(old_dir.initial_capacity),
      statement_dir_vec(std::move(old_dir.statement_dir_vec)) {}

    /* Overloaded operator[]. Both for getting a const and a mutable entry. */

    /* Returns a mutable Lvalue reference to an entry, directly modifyable. */
    Statement_Directory_Entry& operator[](const size_t entry_ix)
    {
        return statement_dir_vec[entry_ix];
    }
    /* Returns a const Lvalue reference to an entry, not modifyable. */
    const Statement_Directory_Entry& operator[](const size_t entry_ix) const
    {
        return statement_dir_vec[entry_ix];
    }

    /* Emplace an entry at the back. */
    inline void
    emplace_back(const size_t code_block_ix_in, const size_t stmt_ix_in,
                 const size_t root_ast_node_arena_offset_in)
    {
        statement_dir_vec.emplace_back(code_block_ix_in, stmt_ix_in,
                                       root_ast_node_arena_offset_in);
        return;
    }

    inline size_t size() const
    {
        return statement_dir_vec.size();
    }
};


/*----------------------------------------------------------------------------*/
