# Overview
In this project, I'm making my own programming language and writing its compiler
in C++ completely from scratch. I've designed the syntax and grammar, written my
own lexing, AST construction, SSA IR code generator and assembly code generator.
Doing the work by myself with my own two hands and brain, not using AI-generated
code or "vibe coding", as I believe that only diminishes an engineer's skills.

The primary aim of this project is to be a substantial learning experience for me
in the topics of compiler design, C++ and assembly language programming and low-level
CPU microarchitecture-driven optimization techniques, and to have lots of fun of course.

So far the language only has assignments from a literal, another variable or from
a possibly multi-level binary operation, consisting of plus, minus, multiply or divide.

The first assignment to a named variable is its declaration, there are no declarations
without initialization. Binary sub-operations must all have their own set of parentheses.

Example code:
a = 5;
b = a * 100;
var1 = ((a + 100) * b) - (a * 2);

A machine description and assembly code generation is only implemented for x86_64
right now, but it will have aarch64 assembly code generation too.

Despite only having variables of a single type (unsigned 64-bit integer) and
3 types of assignments, setting up all the infrastructure, data structures
and algorithms needed for lexing, AST construction, SSA IR code generation and
x86_64 assembly code generation for this has already reached 3500 lines of C++ code.

After adding some more basics like function calls, a small number of Linux system calls,
IF-ELSE statements and a looping construct, I will stop adding features for a while and
implement a machine description and assembly code generation for ARM64 as well.

In the compiler backend, I've designed it to have one Basic Machine Description and
multiple Special Machine Descriptions per CPU architecture. The basic one allows the
compiler to emit assembly code that will run on a wide range of processors, while the
Special Machine Description exposes CPU microarchitecture details specific to one model
or generation of a particular architecture and lets the compiler optimize the assembly
code with instructions that are available to that architecture family and later ones.
For now, a basic machine description for x86_64 CPUs is implemented. More to come.
