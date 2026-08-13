#pragma once

/*----------------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------------*/

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
