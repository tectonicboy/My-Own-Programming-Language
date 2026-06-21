/* The benefit of the Lexer being a class is that we can elegantly spawn
 * multiple lexers consuming different source files in multiple threads at once.
 *
 * Token type can be readily deduced from the first character of the lexeme.
 * If it's a digit, the token is an integer literal.
 * If it's alphabetical or an underscore, the token is an identifier.
 * Identifier tokens promote to KEYWORD type if they match a keyword.
 * The other token types are all one character long: semicolon,
 * an expression operator in { +, -, *, / } and parentheses.
 * All whitespace is ignored.
 *
 * By knowing the type, we know when that token ends, since each token type
 * is described by a non-overlapping set of characters. We know exactly
 * which characters can be right after the last character too. This makes
 * lexing pretty simple to explain and reason about.
 */
class Lexer
{
private:

    uint32_t current_lexeme_len;

    uint32_t current_token_type_ix;
    uint32_t current_line_ix;
    uint32_t current_col_ix;

    size_t cursor;

    const std::string source_code;
    const size_t source_code_len;

public:

    std::vector<Token> collected_tokens;

    /* Notice the usage of std::move(). This is move semantics. Explained in
     * assets/notes in the C++ notes.
     */

    Lexer(const std::string&& src)
    : current_line_ix(0), current_col_ix(0),
      source_code(std::move(src)), source_code_len(source_code.length())
    {
        /* Reserve initial space in the std::vector for 10 thousand tokens. */
        /* This avoids unwanted hidden heap allocations by the vector.      */
        collected_tokens.reserve(10'000);
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
