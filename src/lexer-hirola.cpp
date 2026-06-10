#include <string>
#include <vector>
#include <array>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cctype>

#include "token.h"

/* The benefit of the Lexer being a class is that we can elegantly spawn
 * multiple lexers consuming different source files in multiple threads at once.
 */
class Lexer
{
private:

    uint32_t current_lexeme_len;

	/* Token type can be readily deduced from the first character of the lexeme.
	 * If it's a digit, the token is an integer literal.
	 * If it's alphabetical or an underscore, the token is an identifier.
     * Identifier tokens promote to KEYWORD type if they match a keyword.
	 * The other token types are all one character long: semicolon,
	 * an expression operator in { +, -, *, / } and parentheses.
     * All whitespace is ignored.
     *
     * By knowing the type, we know when that token ends, since each token type
     * is described by a non-overlapping set of characters. We know exactly
     * which characters can be right after the last character too.
     */
    uint32_t current_token_type_ix;
    uint32_t current_line_ix;
    uint32_t current_col_ix;


public:

    Lexer() : current_line_ix(0), current_col_ix(0) {}

    std::vector<Token> Tokenize_Source_Code(const std::string source_code)
    {
        const size_t source_code_len = source_code.length();

        /* Temporary for when checking identifiers for keyword matches. */
        std::string_view temp_identifier_view;

        /* Reserve initial space in the std::vector for 10 thousand tokens. */
        /* This avoids unwanted hidden heap allocations by the vector.      */
        std::vector<Token> collected_tokens;
        collected_tokens.reserve(10'000);

        std::cout << "Lexer running for source_code_len = " << source_code_len
            << "\n";

        for(size_t i = 0; i < source_code_len; ){
            //std::cout << "Lexer main loop: i = " << i << "\n";
            /* First character of current lexeme. Do quick token types first. */
            if (source_code[i] == ' ' || source_code[i] == '\t')
            {
                ++current_col_ix;
                ++i;
            }
            else if(source_code[i] == '\n')
            {
                ++current_line_ix;
                current_col_ix = 0;
                ++i;
            }
            else if (source_code[i] == ';')
            {
                /* std::vector.emplace_back() constructs the object inside the
                 * memory of the vector's buffer of elements, which is better
                 * here than push_back() which COPIES into the vector's buffer
                 * of elements an already-construcated object.
                 */

                collected_tokens.emplace_back
                    (std::string_view((const char*)&(source_code[i]), 1),
                     current_line_ix, current_col_ix, TOKEN_TYPE_SEMICOLON);
                ++current_col_ix;
                ++i;
            }
            else if (source_code[i] == '(')
            {
                collected_tokens.emplace_back
                    (std::string_view((const char*)&(source_code[i]), 1),
                     current_line_ix, current_col_ix, TOKEN_TYPE_OPEN_PAREN);
                ++current_col_ix;
                ++i;
            }

            else if (source_code[i] == ')')
            {
                collected_tokens.emplace_back
                    (std::string_view((const char*)&(source_code[i]), 1),
                     current_line_ix, current_col_ix, TOKEN_TYPE_CLOSE_PAREN);
                ++current_col_ix;
                ++i;
            }
            else if (   source_code[i] == '+' || source_code[i] == '-'
                     || source_code[i] == '*' || source_code[i] == '/'
                     || source_code[i] == '='
                    )
            {
                collected_tokens.emplace_back
                    (std::string_view((const char*)&(source_code[i]), 1),
                     current_line_ix, current_col_ix, TOKEN_TYPE_OPERATOR);
                ++current_col_ix;
                ++i;
            }
            else if (isdigit(source_code[i]))
            {
                current_lexeme_len = 1;
                for(auto j = i + 1; j < source_code_len; ++j){
                    if( ! isdigit(source_code[j]) ){
                        collected_tokens.emplace_back
                            (std::string_view((const char*)&(source_code[i]),
                                current_lexeme_len),
                             current_line_ix,
                             current_col_ix,
                             TOKEN_TYPE_NUM_LITERAL);
                        break;
                    }
                    ++current_lexeme_len;
                }
                i += current_lexeme_len;
                current_col_ix += current_lexeme_len;
            }
            else if (isalpha(source_code[i]) || (source_code[i] == '_'))
            {
                current_lexeme_len = 1;
                for(size_t j = i + 1; j < source_code_len; ++j){

                    bool keyword_matched = 0;

                    if( !isalpha(source_code[j]) && !(source_code[j] == '_' ))
                    {
                        /* If a keyword is matched, token type is KEYWORD. */
                        temp_identifier_view = std::string_view(
                            (const char*)&(source_code[i], current_lexeme_len));

                        for(size_t k = 0; k < reserved_keywords; ++k){
                            if(temp_identifier_view
                                == reserved_keyword_strings[k])
                            {
                                collected_tokens.emplace_back
                                    (std::string_view(
                                        (const char*)&(source_code[i]),
                                        current_lexeme_len),
                                    current_line_ix,
                                    current_col_ix,
                                    TOKEN_TYPE_KEYWORD);
                                keyword_matched = 1;
                                break;
                            }
                        }

                        if(!keyword_matched){
                            collected_tokens.emplace_back
                               (std::string_view((const char*)&(source_code[i]),
                                    current_lexeme_len),
                                current_line_ix,
                                current_col_ix,
                                TOKEN_TYPE_IDENTIFIER);
                        }

                        break;
                    }
                    if(j == source_code_len - 1){
                        /* At this point, it must be PROG_END to be correct. */
                        temp_identifier_view = std::string_view(
                            (const char*)&(source_code[i]),
                            current_lexeme_len + 1);

                        if(temp_identifier_view
                            != reserved_keyword_strings[KEYWORD_PROG_END])
                        {
                            std::cout <<
                             "ERROR: Program end not found. Insert PROG_END.\n";
                            std::cout << "Last token seen in source code: "
                                      << temp_identifier_view << "\n";

                            std::abort();
                        }

                        collected_tokens.emplace_back
                            (std::string_view((const char*)&(source_code[i]),
                                              current_lexeme_len + 1),
                             current_line_ix,
                             current_col_ix,
                             TOKEN_TYPE_KEYWORD);

                        break;
                    }
                    ++current_lexeme_len;
                }
                i += current_lexeme_len;
                current_col_ix += current_lexeme_len;
            }
            else
            {
                std::cout << "\nError: On line " << current_line_ix + 1 << ":"
                          << current_col_ix + 1
                          << "  --  Unrecognized source code character: "
                          << source_code[i]
                          << "\n\n";
                std::abort();
            }
        }

        /* DEBUG ONLY. Print all collected tokens. */
        std::cout << "\nLexer finished! Printing collected program tokens:\n";
        for(size_t i = 0; i < collected_tokens.size(); ++i){
            std::cout << "Printing token " << i << "\n";
            collected_tokens[i].Print_Token_Info();
        }
        std::cout << "\nTotal tokens: " << collected_tokens.size() << "\n\n";

        return collected_tokens;
    }
};

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

    Lexer lexer1;

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv1);
    lexer1.Tokenize_Source_Code(first_program);
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv2);

    std::cout << "Time taken: "
              << ((tv2.tv_nsec - tv1.tv_nsec) / (double)1000.0)
              << " microseconds.\n\n";
}
