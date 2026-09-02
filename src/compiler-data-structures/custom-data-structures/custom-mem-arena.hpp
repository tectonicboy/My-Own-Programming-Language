/* Custom memory arena. It holds different sized, but related, objects all in
 * one place, contiguously, to boost spatial cache locality. Examples used for
 * the compiler's own C++ source code include the IR Instructions Arena, which
 * contains individual emitted IR Instructions, and the AST Arena, which holds
 * the entire generated AST, with contiguous nodes, for a given source program.
 *
 * The add_entry() member function uses a template type T for the type of
 * object that is to be added, as well as a template parameter pack Args...
 * which describes the arity and individual types of arguments that are given
 * to the add_entry() function, in order for them to be passed to the new
 * entry's constructor via std::forward<>(), allowing it to be constructed
 * in place into its slot in the memory arena using C++ placement new.
 *
 * In all use cases, a special bookkeeping data structure contains the byte
 * offset of elements present in the arena.
 *
 * For instance, in the IR Instructions Arena, the byte offset of each
 * IR Instruction object present in it is contained in a data structure called
 * the IR Instructions Directory, wherein each entry holds the Code Block and
 * source code Statement index which the given IR Instruction was emitted for,
 * the byte offset of this IR Instruction into the memory arena and other
 * compiler bookkeeping.
 */
class MEM_Arena
{
private:
    constexpr static size_t initial_capacity_bytes = 32'000;
    constexpr static size_t capacity_multiplier    = 4;

public:
    uint8_t*    arena_ptr;
    size_t      wr_offset;
    size_t      curr_capacity;
    std::string arena_name;

    /* Constructor. */
    explicit MEM_Arena(std::string name_in)
    : wr_offset(0), curr_capacity(initial_capacity_bytes), arena_name(name_in)
    {
        arena_ptr =
         (uint8_t*)aligned_alloc(cache_line_size_bytes, initial_capacity_bytes);

        if(arena_ptr == nullptr) [[unlikely]]
        {
            std::cout << "[ERR] Allocating " << curr_capacity
                      << " bytes for arena named " << arena_name << " failed.\n"
                      << "Aborting compilation.\n\n";
            std::abort();
        }
        memset(arena_ptr, 0x00, initial_capacity_bytes);
    }

    /* Move constructor.
     *
     * Used, for instance, when the AST Arena gets fully transferred from the
     * AST Generation Orchestrator to the IR Generation Orchestrator.
     * Make sure to invalidate the old arena's attributes and pointer.
     */
    MEM_Arena(MEM_Arena&& other_arena)
    : arena_ptr(other_arena.arena_ptr), wr_offset(other_arena.wr_offset),
      curr_capacity(other_arena.curr_capacity),
      arena_name(other_arena.arena_name)
    {
        other_arena.arena_ptr     = nullptr;
        other_arena.wr_offset     = 0;
        other_arena.curr_capacity = 0;
        other_arena.arena_name    = "INVALIDATED_MEMORY_ARENA";
    };

    /* Destructor. */
    ~MEM_Arena(void)
    {
        zero_out_arena();
        free(arena_ptr);
        return;
    }

    /* Returns the byte offset which the new entry was placed at. */
    template <typename T, typename... Args>
    size_t add_entry(Args&&... args)
    {
        /* Align if needed. If there isn't enough remaining arena capacity,
         * resize it. Use C++ placement new to construct the new arena entry.
         */
        while( ((uintptr_t)(arena_ptr + wr_offset)) % alignof(T) != 0 )
            ++wr_offset;

        while(wr_offset + sizeof(T) > curr_capacity) [[unlikely]]
            increase_arena_capacity();

        new (arena_ptr + wr_offset) T(std::forward<Args>(args)...);
        wr_offset += sizeof(T);

        return (wr_offset - sizeof(T));
    }

    void increase_arena_capacity(void)
    {
        uint8_t* temp_resizing_ptr = (uint8_t*) aligned_alloc
            (cache_line_size_bytes, curr_capacity * capacity_multiplier);

        if(temp_resizing_ptr == nullptr) [[unlikely]]
        {
            std::cout << "[ERR] Allocating "
                      << curr_capacity * capacity_multiplier
                      << " bytes for arena named " << arena_name << " failed.\n"
                      << "Aborting compilation.\n\n";
            std::abort();
        }
        memcpy(temp_resizing_ptr, arena_ptr, curr_capacity);
        free(arena_ptr);
        arena_ptr = temp_resizing_ptr;
        curr_capacity *= capacity_multiplier;
        return;
    }

    void zero_out_arena(void)
    {
        memset(arena_ptr, 0x00, curr_capacity);
        return;
    }
};
