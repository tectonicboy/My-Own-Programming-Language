/* THROWAWAY TEMPORARY EXAMPLE C++ CODE TO DRIVE CURRENT COMPILER PHASES. */

#include <new>
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
#include <stdlib.h>

#include "../auxilliary-header-files/compiler-constants.hpp"

#include "../compiler-data-structures/front-end-structures.hpp"
#include "../compiler-data-structures/middle-end-structures.hpp"
#include "../compiler-data-structures/back-end-structures.hpp"
#include "../compiler-data-structures/custom-data-structures/custom-mem-arena.hpp"

#include "../compiler-phases/front-end/lexer.hpp"
#include "../compiler-phases/front-end/ast.hpp"
#include "../compiler-phases/front-end/parser.hpp"
#include "../compiler-phases/middle-end/ir-instructions.hpp"
#include "../compiler-phases/middle-end/ir-generator.hpp"

void grab_source_code_string(char* source_file_name, std::string& target_str)
{
    size_t      bytes_read;
    size_t      file_size;
    uint8_t*    source_code_buf;
    FILE*       hirola_program_fd;

    if( ! (hirola_program_fd = fopen(source_file_name, "r")) ) [[unlikely]]
    {
        perror("Could not open Hirola source code file: ");
        std::abort();
    }
    if( fseek(hirola_program_fd, 0, SEEK_END) == -1 ) [[unlikely]]
    {
        perror("Could not set source file position indicator to end of file: ");
        std::abort();
    }
    if( (file_size = ftell(hirola_program_fd)) == 0 ) [[unlikely]]
    {
        perror("Size of Hirola source code file is 0 bytes. Cannot compile.\n");
        std::abort();
    }
    if( fseek(hirola_program_fd, 0, SEEK_SET) == -1 ) [[unlikely]]
    {
        perror("Could not set source file position indicator back to start: ");
        std::abort();
    }
    if( (source_code_buf = (uint8_t*)malloc(file_size)) == NULL ) [[unlikely]]
    {
        perror("Heap allocation failed for source file contents buffer: ");
        std::abort();
    }

    bytes_read = fread(source_code_buf, 1, file_size, hirola_program_fd);

    if(bytes_read != file_size || ferror(hirola_program_fd))
    {
        printf("Error while reading source code file contents.\n");
        std::abort();
    }

    target_str = std::string((const char*)source_code_buf, file_size);

    free(source_code_buf);
    return;
}

int main(int argc, char* argv[])
{
    struct      timespec tv1;
    struct      timespec tv2;
    std::string source_code_str;

    /* Compiler called using:  hirola input_file.hir output_file */
    if(argc != 3) [[unlikely]]
    {
        printf("Specify Hirola source code filepath, then output filepath.\n");
        std::abort();
    }

    grab_source_code_string(argv[1], source_code_str);

    std::cout << "Tokenizing the following program:\n\n"
              << source_code_str << "\n\n";

    Lexer lexer1(std::move(source_code_str));

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv1);
    lexer1.Tokenize_Source_Code();
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv2);

    std::cout << "\nLexer finished! Printing collected program tokens:\n";

    for(size_t i = 0; i < lexer1.token_array.size(); ++i){
        std::cout << "Printing token " << i << "\n";
        lexer1.token_array[i].Print_Token_Info();
    }

    std::cout << "\nTotal tokens: " << lexer1.token_array.size() << "\n\n";

    std::cout << "Printing collected Code Blocks:\n";

    for(size_t i = 0; i < lexer1.code_block_dir.size(); ++i)
    {
        lexer1.code_block_dir[i].print_code_block_info();
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
        (std::move(lexer1.code_block_dir),
         std::move(lexer1.token_array),
         std::move(parsing_quotas));

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv1);

    /* Gives it the vector with one element: {0} as the parsing job quota. */
    my_parsing_orchestrator.spawn_parser
        (my_parsing_orchestrator.parsing_quotas[0]);

    clock_gettime(CLOCK_MONOTONIC_RAW, &tv2);

    std::cout << "Time taken PARSER: "
              << ((tv2.tv_nsec - tv1.tv_nsec) / (double)1000.0)
              << " microseconds.\n\n";

    std::cout << "Reconstructing the source program from the AST:\n\n";

    size_t entries = my_parsing_orchestrator.statement_dir.size();
    size_t arena_offset;
    AST_Node_Statement_Assignment* stmt_ast_node = nullptr;

    for(size_t i = 0; i < entries; ++i)
    {
        arena_offset = my_parsing_orchestrator.statement_dir[i]
                        .root_ast_node_arena_offset;

        stmt_ast_node = (AST_Node_Statement_Assignment*)
                   (my_parsing_orchestrator.ast_arena.arena_ptr + arena_offset);

        stmt_ast_node->print_node();

        std::cout << "\n";
    }
    std::cout << "\n";
    IR_Generation_Orchestrator my_ir_generation_orchestrator
        (std::move(my_parsing_orchestrator.ast_arena),
         std::move(my_parsing_orchestrator.statement_dir),
         std::move(my_parsing_orchestrator.parsing_quotas));
    my_ir_generation_orchestrator.spawn_IR_generator
        (my_ir_generation_orchestrator.IR_generation_quotas[0]);
    entries = my_ir_generation_orchestrator.IR_instructions_dir.size();
    IR_Instructions_Directory_Entry* entry;
    size_t which_ir_insn;
    uint8_t* arena =
        my_ir_generation_orchestrator.IR_instructions_arena.arena_ptr;
    ir_insn_equate* insn_equ = nullptr;
    ir_insn_add*    insn_add = nullptr;
    ir_insn_sub*    insn_sub = nullptr;
    ir_insn_mul*    insn_mul = nullptr;
    ir_insn_div*    insn_div = nullptr;

    for(size_t i = 0; i < entries; ++i)
    {
        entry = &(my_ir_generation_orchestrator.IR_instructions_dir[i]);
        arena_offset = entry->ir_insn_arena_offset;
        which_ir_insn = entry->which_ir_instruction;
        if(which_ir_insn == IR_INSN_EQUATE)
        {
            insn_equ = (ir_insn_equate*)(arena + arena_offset);
            insn_equ->print_ir_insn();
        }
        else if(which_ir_insn == IR_INSN_ADD)
        {
            insn_add = (ir_insn_add*)(arena + arena_offset);
            insn_add->print_ir_insn();
        }
        else if(which_ir_insn == IR_INSN_SUB)
        {
            insn_sub = (ir_insn_sub*)(arena + arena_offset);
            insn_sub->print_ir_insn();
        }
        else if(which_ir_insn == IR_INSN_MUL)
        {
            insn_mul = (ir_insn_mul*)(arena + arena_offset);
            insn_mul->print_ir_insn();
        }
        else if(which_ir_insn == IR_INSN_DIV)
        {
            insn_div = (ir_insn_div*)(arena + arena_offset);
            insn_div->print_ir_insn();
        }
    }

    return 0;
}
