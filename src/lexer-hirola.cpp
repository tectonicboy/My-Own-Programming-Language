#include <string>
#include <vector>
#include <array>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cctype>

#include "token.h"
#include "lexer.h"

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
    current_col_ix = 0;
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
    for(auto j = cursor + 1; j < source_code_len; ++j){
        if( ! isdigit(source_code[j]) )
            break;
        ++current_lexeme_len;
    }

    /* Two ways to get here: Either a full token was recorded, or the end of the
     * program was reached. In both cases, simply record the token and move on.
     */

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

    for(size_t j = cursor + 1; j < source_code_len; ++j){
        if( !isalpha(source_code[j]) && !(source_code[j] == '_' ))
            break;
        ++current_lexeme_len;
    }

    /* Two ways to get here: Either a full token was recorded, or the end of the
     * program was reached. In both cases, simply record the token and move on.
     */

    /* Store the token string view temporarily here. */
    std::string_view temp_identifier_view = std::string_view
        ((const char*)&(source_code[cursor]), current_lexeme_len);

    /* Check if it's a language keyword. */
    for(size_t k = 0; k < reserved_keywords; ++k){
        if(temp_identifier_view == reserved_keyword_strings[k])
        {
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
            std::cout << "\nError: On line " << current_line_ix + 1 << ":"
                      << current_col_ix + 1
                      << "  --  Unrecognized source code character: "
                      << source_code[cursor]
                      << "\n\n";
            std::abort();
        }
    }
    return;
}

int main()
{
    struct timespec tv1, tv2;

    std::string first_program =
        "x = 5;\n"
        "base = (x + 105);\n"
        "kk = ( (base * 1000000) - (x * x) );\n"
        "c = (((x) - (base * base)) / (kk) )\n"
        "PROG_END";

    std::cout << "Tokenizing the following program:\n\n"
              << first_program << "\n\n";

    Lexer lexer1(std::move(first_program));

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv1);
    lexer1.Tokenize_Source_Code();
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv2);

    std::cout << "\nLexer finished! Printing collected program tokens:\n";

    for(size_t i = 0; i < lexer1.collected_tokens.size(); ++i){
        std::cout << "Printing token " << i << "\n";
        lexer1.collected_tokens[i].Print_Token_Info();
    }

    std::cout << "\nTotal tokens: " << lexer1.collected_tokens.size() << "\n\n";

    std::cout << "Time taken: "
              << ((tv2.tv_nsec - tv1.tv_nsec) / (double)1000.0)
              << " microseconds.\n\n";
    return 0;
}
