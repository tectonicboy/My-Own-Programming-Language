
/* Receives from a Parsing Orchestrator:
 *
 *  - Populated Auxilliary Code Block Statement Directory
 *  - Populated AST Arena
 *  - Populated Symbol Table
 *
 * Initializes:
 *
 *  - A new empty IR Instructions Arena
 *  - Maximum bytes usable in the IR Instructions Arena
 *  - Currently used bytes in the IR Instructions Arena = zero
 *  - Pointer to next free region of the IR Instructions Arena = start of it
 *
 * Gives jobs to one or more IR Generators by giving them quotas for which
 * Code Block's statement AST Nodes to emit IR instructions for and their own
 * regions of the IR Instructions Arena to emit their IR code to.
 */
class ir_generation_orchestrator {

public:



}


/* Receives from an IR Generation Orchestrator:
 *
 *  - Quota: vector<size_t> of block indices whose AST statements to emit IR of.
 *  - Populated Auxilliary Code Block Statement Directory
 *  - Populated AST Arena
 *  - Populated Symbol Table
 *  - Pointer to empty region of the IR Instructions Arena
 *  - How much memory it's allowed to use in the IR Instructions Arena
 *  - Pointer to which it should write how much memory it ended up using.
 *
 * Produces:
 *
 *  - populated IR Instructions Arena region.
 */
class ir_generator
