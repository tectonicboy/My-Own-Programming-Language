/*----------------------------------------------------------------------------*/
class ir_insn_equate
{
public:

    std::string lhs;
    std::string rhs;

    /* Constructor. */
    explicit ir_insn_equate (std::string lhs_in, std::string rhs_in)
    : lhs(lhs_in), rhs(rhs_in) {}

    void print_ir_insn(void) const
    {
        std::cout << lhs << " = " << rhs << "\n";
    }
};

/*----------------------------------------------------------------------------*/

class ir_insn_add
{

public:
    std::string lhs_operand;
    std::string rhs_operand;
    std::string target;

    /* Constructor. */
    explicit ir_insn_add(std::string lhs_operand_in, std::string rhs_operand_in,
                         std::string target_in)
    : lhs_operand(lhs_operand_in), rhs_operand(rhs_operand_in),
      target(target_in) {}

    void print_ir_insn(void) const
    {
        std::cout << target << " = " << lhs_operand << " + " << rhs_operand
                  << "\n";
    }
};

/*----------------------------------------------------------------------------*/

class ir_insn_sub
{

public:
    std::string lhs_operand;
    std::string rhs_operand;
    std::string target;

    /* Constructor. */
    explicit ir_insn_sub(std::string lhs_operand_in, std::string rhs_operand_in,
                         std::string target_in)
    : lhs_operand(lhs_operand_in), rhs_operand(rhs_operand_in),
      target(target_in) {}

    void print_ir_insn(void) const
    {
        std::cout << target << " = " << lhs_operand << " - " << rhs_operand
                  << "\n";
    }
};

/*----------------------------------------------------------------------------*/

class ir_insn_mul
{

public:
    std::string lhs_operand;
    std::string rhs_operand;
    std::string target;

    /* Constructor. */
    explicit ir_insn_mul(std::string lhs_operand_in, std::string rhs_operand_in,
                         std::string target_in)
    : lhs_operand(lhs_operand_in), rhs_operand(rhs_operand_in),
      target(target_in) {}

    void print_ir_insn(void) const
    {
        std::cout << target << " = " << lhs_operand << " * " << rhs_operand
                  << "\n";
    }
};

/*----------------------------------------------------------------------------*/

class ir_insn_div
{

public:
    std::string lhs_operand;
    std::string rhs_operand;
    std::string quotient;

    /* Constructor. */
    explicit ir_insn_div(std::string lhs_operand_in, std::string rhs_operand_in,
                         std::string quotient_in)
    : lhs_operand(lhs_operand_in), rhs_operand(rhs_operand_in),
      quotient(quotient_in) {}

    void print_ir_insn(void) const
    {
        std::cout << quotient << " = "
                  << lhs_operand << " / " << rhs_operand << "\n";
    }
};

/*----------------------------------------------------------------------------*/
