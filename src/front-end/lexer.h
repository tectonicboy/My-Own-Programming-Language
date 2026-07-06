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
    size_t current_block_start_token_ix;
    size_t current_block_end_token_ix;
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

/* Small functions describing how to process each type of Token. */

__attribute__((always_inline))
inline void Lexer::lex_whitespace(void)
{
    ++current_col_ix;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_newline(void)
{
    ++current_line_ix;
    current_col_ix = 1;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_semicolon(void)
{
    collected_tokens.emplace_back
        (std::string_view((const char*)&(source_code[cursor]), 1),
         current_line_ix, current_col_ix, TOKEN_TYPE_SEMICOLON);
    ++current_col_ix;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_open_paren(void)
{
    collected_tokens.emplace_back
        (std::string_view((const char*)&(source_code[cursor]), 1),
         current_line_ix, current_col_ix, TOKEN_TYPE_OPEN_PAREN);
    ++current_col_ix;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_close_paren(void)
{
    collected_tokens.emplace_back
        (std::string_view((const char*)&(source_code[cursor]), 1),
         current_line_ix, current_col_ix, TOKEN_TYPE_CLOSE_PAREN);
    ++current_col_ix;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_operator(void)
{
    collected_tokens.emplace_back
        (std::string_view((const char*)&(source_code[cursor]), 1),
         current_line_ix, current_col_ix, TOKEN_TYPE_OPERATOR);
    ++current_col_ix;
    ++cursor;
}

__attribute__((always_inline))
inline void Lexer::lex_num_literal_uint(void)
{
    current_lexeme_len = 1;
    size_t j;

    for(j = cursor + 1; j < source_code_len; ++j){
        if( ! isdigit(source_code[j]) )
            break;
        ++current_lexeme_len;
    }

    /* Two ways to get here: Either a full token was recorded, or the end of the
     * program was reached. In both cases, simply record the token and move on.
     *
     * If program end is reached here, throw a syntax error for not closing
     * the code block with BLOCK_END keyword.
     */

    if(j == source_code_len)
    [[unlikely]]
    {
        std::cout
             << "\n\n"
             << "Syntax Error: Reached end of program without closing the\n"
             << "              final code block. Use BLOCK_END keyword.\n";
        std::abort();
    }

    collected_tokens.emplace_back(std::string_view(
                                     (const char*)&(source_code[cursor]),
                                     current_lexeme_len),
                                  current_line_ix,
                                  current_col_ix,
                                  TOKEN_TYPE_NUM_LITERAL_UINT);

    cursor += current_lexeme_len;
    current_col_ix += current_lexeme_len;
}

__attribute__((always_inline))
inline void Lexer::lex_identifier_and_keyword(void)
{
    current_lexeme_len   = 1;
    bool keyword_matched = false;
    size_t j;

    for(j = cursor + 1; j < source_code_len; ++j){
        if( !isalpha(source_code[j]) && !(source_code[j] == '_' ))
            break;
        ++current_lexeme_len;
    }

    /* Two ways to get here: Either a full token was recorded, or the end of the
     * program was reached. In both cases, simply record the token and move on.
     *
     * If end of program is reached, make sure it was a BLOCK_END keyword
     * closing the final Code Block and give a syntax error if not.
     */

    /* Store the token string view temporarily here. */
    std::string_view temp_identifier_view = std::string_view
        ((const char*)&(source_code[cursor]), current_lexeme_len);

    if(j == source_code_len)
    [[unlikely]]
    {
        if(source_code.compare
            (cursor,
             current_lexeme_len,
             reserved_keyword_strings[KEYWORD_BLOCK_END]) != 0)
        {
            std::cout
                 << "\n\n"
                 << "Syntax Error: Reached end of program without closing the\n"
                 << "              final code block. Use BLOCK_END keyword.\n";
            std::abort();
        }
    }

    /* Check if it's a language keyword. */
    for(size_t k = 0; k < total_keywords; ++k){
        if(temp_identifier_view == reserved_keyword_strings[k])
        {
            /* If the keyword was BLOCK_START or BLOCK_END, update bookkeeping
             * regarding Code Blocks in the source code.
             */
            if(k == KEYWORD_BLOCK_START)
            {
                if(inside_code_block == true)
                {
                    std::cout
                         << "\n\n"
                         << "Syntax Error: Starting a new code block before\n"
                         << "              having finished the last one.\n"
                         << "Line: " << current_line_ix << "\n\n";
                    std::abort();
                }
                inside_code_block = true;
                aux_code_block_directory.emplace_back
                    (Code_Block(collected_tokens.size(), 0, 0));
            }
            else if(k == KEYWORD_BLOCK_END)
            {
                if(inside_code_block == false)
                {
                    std::cout
                         << "\n\n"
                         << "Syntax Error: Finishing a code block that was\n"
                         << "              never started to begin with.\n"
                         << "Line: " << current_line_ix << "\n\n";
                    std::abort();
                }
                inside_code_block = false;
                aux_code_block_directory.back().end_token_index
                    = collected_tokens.size();
            }

            collected_tokens.emplace_back
                (std::string_view(temp_identifier_view),
                current_line_ix,
                current_col_ix,
                TOKEN_TYPE_KEYWORD);

            keyword_matched = true;
            break;
        }
    }
    /* It's not a keyword. Add it as an identifier token. */
    if(!keyword_matched)
    {
        collected_tokens.emplace_back(std::string_view(temp_identifier_view),
                                      current_line_ix, current_col_ix,
                                      TOKEN_TYPE_IDENTIFIER);
    }
    /* Lastly, advance the cursor and column and continue lexing more tokens. */
    cursor += current_lexeme_len;
    current_col_ix += current_lexeme_len;
}

/* Primary tokenizer function. */
void Lexer::Tokenize_Source_Code(void)
{
    std::cout << "Lexer running for source code length: "
              << source_code_len << "\n";

    for(cursor = 0; cursor < source_code_len; )
    {
        /* One or more Code Blocks make up the program. Enforce this here and
         * in the code for reaching the end of the program (so, all token
         * processor functions for multi-character token types), as well as at
         * the code for seeing BLOCK_START and BLOCK_END keywords. This check
         * here only peeks into the source text, not producing tokens.
         */
        if(inside_code_block == false)
        [[unlikely]]
        {
            constexpr size_t needed_len =
                strlen(reserved_keyword_strings[KEYWORD_BLOCK_START]);

            bool next_is_block_start_keyword =
                   (source_code_len - cursor > needed_len)
                && (source_code.compare
                        (cursor,
                         needed_len,
                         reserved_keyword_strings[KEYWORD_BLOCK_START]) == 0);
            if( ! next_is_block_start_keyword){
                std::cout
                     << "\n\n"
                     << "Syntax Error: Source code not inside a Code Block.\n"
                     << "              Surround all logical parts of your\n"
                     << "              code with BLOCK_START and BLOCK_END.\n"
                     << "Line: " << current_line_ix << "\n\n";
                std::abort();
            }

        }

        /* All these helper function calls are always inlined. */

        if (source_code[cursor] == ' ' || source_code[cursor] == '\t')
            lex_whitespace();

        else if(source_code[cursor] == '\n')
            lex_newline();

        else if (source_code[cursor] == ';')
            lex_semicolon();

        else if (source_code[cursor] == '(')
            lex_open_paren();

        else if (source_code[cursor] == ')')
            lex_close_paren();

        else if (   source_code[cursor] == '+' || source_code[cursor] == '-'
                 || source_code[cursor] == '*' || source_code[cursor] == '/'
                 || source_code[cursor] == '='
                )
            lex_operator();

        else if (isdigit(source_code[cursor]))
            lex_num_literal_uint();

        else if (isalpha(source_code[cursor]) || (source_code[cursor] == '_'))
            lex_identifier_and_keyword();

        else
        {
            std::cout << "\nError: On line " << current_line_ix << ":"
                      << current_col_ix
                      << "  --  Unrecognized source code character: "
                      << source_code[cursor]
                      << "\n\n";
            std::abort();
        }
    }

    /* At the end of program, we must have set the End Token Index of the last
     * seen Code Block to non-zero. If it's still 0 here, BLOCK_END keyword
     * was never seen for the last Code Block, give a syntax error.
     *
     * If it's not seen for a non-last Code Block, this gets caught by the
     * code for seeing the BLOCK_START keyword.
     */
    if(aux_code_block_directory.back().end_token_index == 0)
    {
        std::cout
             << "\n\n"
             << "Syntax Error: End of program reached and the last Code Block\n"
             << "              was never closed with BLOCK_END keyword.\n";
        std::abort();
    }

    return;
}

