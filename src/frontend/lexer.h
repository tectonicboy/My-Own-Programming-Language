/* The benefit of the Lexer being a class is that we can elegantly spawn
 * multiple lexers consuming different source files in multiple threads at once.
 *
 * Token type can be readily deduced from the first character of the lexeme.
 * All whitespace is ignored. No semantic analysis is performed during lexing.
 * Minimal syntax errors are caught like unrecognized tokens and improper
 * containment of statements inside Code Blocks.
 *
 * Knowing the token type, we know when that token ends, since each token type
 * is described by a non-overlapping set of characters. We know exactly
 * which characters can be right after the last character too. This makes
 * lexing simple to reason about and implement.
 *
 * The Lexer does not need to check for Code Block types, it just needs to check
 * for complete code blocks, with START_BLOCK and END_BLOCK, that's all. The
 * Semantic Analyzer will check Code Block types using the Parser-emitted AST.
 */
class Lexer
{
private:

    uint64_t current_lexeme_len;
    uint64_t current_token_type_ix;
    uint64_t current_line_ix;
    uint64_t current_col_ix;
    uint64_t current_block_start_token_ix;
    uint64_t current_block_end_token_ix;
    uint64_t current_block_type_ix;
    size_t   cursor;
    bool     inside_code_block;

    const std::string source_code;
    const size_t      source_code_len;

public:

    std::vector<Token>      collected_tokens;
    std::vector<Code_Block> aux_code_block_directory;

    /* Notice the usage of std::move(). This is move semantics. Explained in
     * assets/notes/cpp-notes.txt
     */

    Lexer(const std::string&& src)
    : current_line_ix(1), current_col_ix(1), inside_code_block(false),
      source_code(std::move(src)), source_code_len(source_code.length())
    {
        /* Reserve initial space in the std::vector for 10 thousand tokens. */
        /* This avoids unwanted hidden heap allocations by the vector.      */
        collected_tokens.reserve(10'000);
        aux_code_block_directory.reserve(100);
    }

    void Tokenize_Source_Code(void);

private:

    /*------------------------------------------------------------------------*/

    /* Functions describing how to process each Token type.                  */
    /* Note the Lexer keeps track of which line and column each token is at. */

    /* Space & TAB: Ignore it, advance cursor. */
    __attribute__((always_inline))
    inline void lex_whitespace(void);

    /* Newline: Ignore it, advance cursor. Advance line. Reset column. */
    __attribute__((always_inline))
    inline void lex_newline(void);

    /* Semicolon: Add the token, advance cursor. */
    __attribute__((always_inline))
    inline void lex_semicolon(void);

    /* Open paranthesis: Add the token, advance cursor. */
    __attribute__((always_inline))
    inline void lex_open_paren(void);

    /* Close parenthesis: Add the token, advance cursor. */
    __attribute__((always_inline))
    inline void lex_close_paren(void);

    /* Operator: Add the token, advance cursor. */
    __attribute__((always_inline))
    inline void lex_operator(void);

    /* Numeric Literal Unsigned Integer: Keep consuming characters until you see
     *                                   a non-numeric. Record the length L.
     *                                   Add the token. Advance cursor by L.
     *                                   If end of the program is reached,
     *                                   record the token and move on.
     */
    __attribute__((always_inline))
    inline void lex_num_literal_uint(void);

    /* Identifiers and keywords: Keep consuming characters until you see a
     *                           non-alphabetic and not an underscore. If it's a
     *                           match with a keyword, add a Keyword token type.
     *                           Else, add an Identifier token type with the
     *                           recoreded length L. Advance cursor by L.
     *                           If end of program is reached, record the token
     *                           and move on.
     */
    __attribute__((always_inline))
    inline void lex_identifier_and_keyword(void);

};
