#include <string>
#include <vector>
#include <array>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unordered_map>


#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"

int main()
{
    struct timespec tv1, tv2;

    std::string first_program =
        "BLOCK_START PROGRAM\n"
        "x = 5;\n"
        "base = (x + 105);\n"
        "kk = ( (base * 1000000) - (x * x));\n"
        "c = ( (x - (base * base)) / kk );\n"
        "BLOCK_END";

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

    std::cout << "Printing collected Code Blocks:\n";

    for(size_t i = 0; i < lexer1.aux_code_block_directory.size(); ++i){
        lexer1.aux_code_block_directory[i].print_code_block_info();
    }

    std::cout << "Time taken LEXER: "
              << ((tv2.tv_nsec - tv1.tv_nsec) / (double)1000.0)
              << " microseconds.\n\n";

    std::cout << "\n\n ***** PARSING ORCHESTRATOR SPAWNING *****\n\n\n";

    /* Ask the ParsingOrchestrator for now to only have one job quota,
     * and in that single job quota, the job is to parse a single Code Block
     * that is to parse the only Code Block available currently in the
     * language, the main program Code Block, block [0]. Later we may have
     * multithreaded parsing and pass several job quotas to the Orchestrator
     * so it can spawn multiple Parsers, one per thread, and give them each
     * one of the parsing quotas given in each std::vector in the vector of
     * vectors. The Orchestrator gets transferred ownership of the Code Block
     * Directory and of the Token Array, from the Lexer object.
     */

    std::vector<std::vector<size_t>> parsing_quotas = {{0}};

    ParsingOrchestrator my_parsing_orchestrator
        (std::move(lexer1.aux_code_block_directory),
         std::move(lexer1.collected_tokens),
         parsing_quotas);

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv1);

    /* Gives it the vector with one element: {0} as the parsing job quota. */
    my_parsing_orchestrator.spawn_parser
        (my_parsing_orchestrator.parsing_quotas[0]);

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv2);

    std::cout << "Time taken PARSER: "
              << ((tv2.tv_nsec - tv1.tv_nsec) / (double)1000.0)
              << " microseconds.\n\n";

    std::cout << "Printing each Assignment Statement AST Node on new lines:\n";

    for(size_t i = 0;
        i < my_parsing_orchestrator.statement_directory_used_entries;
        ++i)
    {
        ((AST_Node_Statement_Assignment*)(my_parsing_orchestrator.ast_arena + std::get<STMT_DIR_NODE_ARENA_OFFSET>(my_parsing_orchestrator.statement_directory[i]) ))
        ->print_node();
        std::cout << "\n";
    }

    return 0;
}
