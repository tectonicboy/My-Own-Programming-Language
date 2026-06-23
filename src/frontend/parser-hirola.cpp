/* Instead of dynamically allocating each AST Node and chasing pointers to get
 * to a particular Node, I devise a custom memory allocation scheme, where all
 * the statements that make up the function that the given AST represents live
 * in their own arena. All expressions live in their own separate arena.
 *
 * A per-AST bookkeeping data structure composed of slots keeps metainformation
 * about each statement and the expressions that make it up. It's a lookup
 * table.
class AST_builder
{
public:



};
