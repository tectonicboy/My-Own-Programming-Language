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

/*
std::string first_program =
    "BLOCK_START PROGRAM\n"
    "x = 5;\n"
    "y = 12;\n"
    "z = 7;\n"
    "alpha = 3;\n"
    "beta = 9;\n"
    "gamma = 14;\n"
    "delta = 2;\n"
    "epsilon = 21;\n"
    "base = (x + 105);\n"
    "scale = (y * 3);\n"
    "offset = (z - x);\n"
    "shift = (alpha + beta);\n"
    "twist = (gamma - delta);\n"
    "fold = (epsilon / alpha);\n"
    "kk = ( (base * 1000000) - (x * x));\n"
    "mm = ( (scale + offset) * 2);\n"
    "nn = ( (kk - mm) / 4);\n"
    "c = ( (x - (base * base)) / kk );\n"
    "d = ( (y + z) * (x - 1));\n"
    "e = ( (d * d) - (c * c));\n"
    "f = ( (e / 2) + 17);\n"
    "g = ( (f - nn) * 3);\n"
    "h = ( (g + base) / (scale + 1));\n"
    "i = ( (h * mm) - (offset * offset));\n"
    "j = ( (i + 1000) / 13);\n"
    "k = ( (j - x) * (y + z));\n"
    "l = ( (k / 7) + (d - e));\n"
    "m = ( (l * l) - 256);\n"
    "n = ( (m + g) / (h - 1));\n"
    "o = ( (n * 9) - (f * f));\n"
    "p = ( (o + j) / (k - mm));\n"
    "q = ( (p - n) * (i + 1));\n"
    "r = ( (q / 3) + (base * scale));\n"
    "s = ( (r - offset) * (y - z));\n"
    "t = ( (s + 42) / ((x + y) + z));\n"
    "u = ( (t * t) - (s * r));\n"
    "v = ( (u + q) / (p - 2));\n"
    "w = ( (v - m) * (l + k));\n"
    "aa = ( (w / 5) + (n - o));\n"
    "bb = ( (aa * aa) - (u * v));\n"
    "cc = ( (bb + w) / (aa - 1));\n"
    "dd = ( (cc - t) * (s + r));\n"
    "ee = ( (dd / 6) + (q * p));\n"
    "ff = ( (ee - n) * (m - l));\n"
    "gg = ( (ff + cc) / (dd - bb));\n"
    "hh = ( (gg * 2) - (aa + bb));\n"
    "ii = ( (hh / 4) + (cc * dd));\n"
    "jj = ( (ii - ee) * (ff + gg));\n"
    "kkb = ( (jj + hh) / (ii - 3));\n"
    "ll = ( (kkb * kkb) - (jj * ii));\n"
    "mmb = ( (ll / 8) + (hh - gg));\n"
    "nnb = ( (mmb - ff) * (ee + dd));\n"
    "oo = ( (nnb + cc) / (bb - aa));\n"
    "pp = ( (oo * 7) - (nnb * mmb));\n"
    "qq = ( (pp / 9) + (ll - kkb));\n"
    "rr = ( (qq - jj) * (ii + hh));\n"
    "ss = ( (rr + gg) / (ff - ee));\n"
    "tt = ( (ss * ss) - (dd * cc));\n"
    "uu = ( (tt / 10) + (bb - aa));\n"
    "vv = ( (uu - oo) * (pp + qq));\n"
    "ww = ( (vv + rr) / (ss - 1));\n"
    "xx = ( (ww * 11) - (uu * tt));\n"
    "yy = ( (xx / 12) + (vv - ww));\n"
    "zz = ( (yy - rr) * (ss + tt));\n"
    "chainOne = ( ( ((x + y) - z) * ((alpha + beta) - gamma) ) + ((delta * epsilon) / shift) );\n"
    "chainTwo = ( ( ( (base + scale) - (offset * twist) ) + (fold / shift) ) - (kk + mm) );\n"
    "chainThree = ( ( ( (nn - oo) + (pp * qq) ) / ( (rr - ss) + (tt * uu) ) ) * (vv - ww) );\n"
    "chainFour = ( ( ( ((aa + bb) + cc) + dd) - ( ((ee + ff) + gg) + hh) ) + ((ii - jj) * (kkb + ll)) );\n"
    "chainFive = ( ( ( (d - e) * (f - g) ) + ( (h - i) * (j - k) ) ) - ( (l - m) * (n - o) ) );\n"
    "chainSix = ( ( ( ( ( (p / q) + (r / s) ) - (t / u) ) + (v / w) ) - (aa / bb) ) + (cc / dd) );\n"
    "chainSeven = ( ( ( (shift + twist) - fold) * ( (base - scale) + offset) ) / ( (kkb - ll) + mmb) );\n"
    "chainEight = ( ( ( ((x * y) * z) - ((alpha * beta) * gamma) ) + ((delta * epsilon) * shift) ) - (twist * fold) );\n"
    "chainNine = ( ( ( (x + 1) * (y + 2) ) - ( (z + 3) * (alpha + 4) ) ) + ( (beta - gamma) * (delta - epsilon) ) );\n"
    "chainTen = ( ( ( ( ( (nnb + oo) - (pp + qq) ) + (rr - ss) ) - (tt + uu) ) + (vv - ww) ) - (xx + yy) );\n"
    "resultOne = ( ( (chainOne + chainTwo) - (chainThree * chainFour) ) + (chainFive / chainSix) );\n"
    "resultTwo = ( ( (chainSeven - chainEight) * (chainNine + chainTen) ) / (zz - yy) );\n"
    "resultThree = ( ( (resultOne + resultTwo) - (xx * yy) ) / ( (zz + ww) - (vv * uu) ) );\n"
    "final = ( ( ( (resultThree * 2) + (kk - mm) ) - ( (nn * oo) / (pp + qq) ) ) + ( (rr - ss) * (tt + uu) ) );\n"
    "BLOCK_END";
*/
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
