#pragma once

/*----------------------------------------------------------------------------*/

/* Lookup table of code blocks types. Examples: function definition, classes. */
/* This separation into token groups allows a Parsing Orchestrator to perform
 * multithreaded parsing by letting one or more code blocks be parsed in
 * parallel by one or more Parsers by simply telling each parser which code
 * block(s) to process by means of the Auxilliary Code Block Token Directory
 * produced by the Lexer alongside the Token Array.
 */
constexpr size_t total_code_block_types = 1;

constexpr uint32_t CODE_BLOCK_TYPE_PROGRAM = 0;

constexpr std::array<const char*, total_code_block_types>
code_block_type_strings = {
    "primary program code block"
};

/*----------------------------------------------------------------------------*/

/* Lookup table of current language reserved keywords. */
constexpr size_t total_keywords = 3;

constexpr uint32_t KEYWORD_BLOCK_START = 0;
constexpr uint32_t KEYWORD_BLOCK_END   = 1;
constexpr uint32_t KEYWORD_PROGRAM     = 2;

constexpr std::array<const char*, total_keywords>
reserved_keyword_strings = {
    "BLOCK_START",
    "BLOCK_END",
    "PROGRAM"
};

/*----------------------------------------------------------------------------*/

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
token_type_strings = {
    "Identifier",
    "Keyword",
    "Open Parenthesis",
    "Close Parenthesis",
    "Operator",
    "Semicolon",
    "Number Literal Unsigned Int"
};

/*----------------------------------------------------------------------------*/

/* Descriptor of a code block. */
class Code_Block {

public:

    uint64_t start_token_index;
    uint64_t end_token_index;
    uint64_t code_block_type_index;

    Code_Block(uint64_t start_ix, uint64_t end_ix, uint64_t type_ix)
    : start_token_index(start_ix), end_token_index(end_ix),
      code_block_type_index(type_ix)
    {
        /* Handle error case: an invalid type index was somehow passed. */
        if(type_ix >= total_code_block_types)
        [[unlikely]]
        {
            std::cout << "CRITICAL: Internal compiler error. [LEXER]\n"
                      << "Code Block constructor: Passed type index "
                      << type_ix << "\n"
                      << "Available type indices: 0 to "
                      << total_code_block_types - 1 << "\n"
                      << "Aborting compilation.\n";
            std::abort();
        }
    }

    void print_code_block_info(void){
        std::cout << "\n--------------------------------------------------"
                  << "\nCode Block Type: "
                  << code_block_type_strings[code_block_type_index]
                  << "\nStart Token Index: " << start_token_index
                  << "\nEnd   Token Index: " << end_token_index
                  << "\n--------------------------------------------------\n";
    }
};

/*----------------------------------------------------------------------------*/

/* Placed the string_view object first, to avoid a padding of empty bytes
 * if it's declared after 2nd or 3rd member. The only other optimal place to
 * have it would be last, after 8+4+4 bytes. The two padding rules are:
 *
 *  - Total object size is padded with empty bytes until it's divisible
 *    by the largest member's size.
 *
 *  - Each member must start at an offset within the object that's divisible
 *    by that member's own size in bytes.
 *
 * That's why placing string_view first or last are the only two choices
 * that avoid padding.
 *
 * For singlethreaded operation, other objects residing in the same cache
 * line WILL NOT have to be re-read after an instruction from that same
 * thread updates one of the objects in that cache line. So keep as many
 * as possible on one cache line.
 *
 * Only for multithreading, this becomes an issue called False Sharing.
 * If another thread needs another object on that same cache line, the
 * entire line has to be written out to RAM and brought back into the cache
 * of the CPU that needs the other object from that cache line. This would
 * become a performance killer, so watch out.
 */
class Token {
public:

    std::string_view  token_value;
    uint64_t          token_line_in_src;
    uint32_t          token_col_in_src;
    uint32_t          token_type_ix;

    Token (std::string_view value_text, uint64_t line_in_src,
           uint32_t col_in_src,         uint32_t type_index)

        :  token_value(value_text),      token_line_in_src(line_in_src),
           token_col_in_src(col_in_src), token_type_ix(type_index)
    {
        /* Handle error case: an invalid type index was somehow passed. */
        if(type_index >= total_token_types)
        [[unlikely]]
        {
            std::cout << "CRITICAL: Internal compiler error. [LEXER]\n"
                      << "Token constructor: Passed token type index "
                      << type_index << "\n"
                      << "Available type indices: 0 to "
                      << total_token_types - 1 << "\n"
                      << "Aborting compilation.\n";
            std::abort();
        }
    }

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
