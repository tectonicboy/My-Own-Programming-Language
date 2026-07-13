.PHONY: lexer

CXXFLAGS += -D_GNU_SOURCE
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -Wcast-align
CXXFLAGS += -Wfloat-equal
CXXFLAGS += -Wformat=2
CXXFLAGS += -Wlogical-op
CXXFLAGS += -Wmissing-include-dirs
CXXFLAGS += -Wpointer-arith
CXXFLAGS += -Wredundant-decls
CXXFLAGS += -Wsequence-point
CXXFLAGS += -Wshadow
CXXFLAGS += -Wswitch
CXXFLAGS += -Wundef
CXXFLAGS += -Wunreachable-code
CXXFLAGS += -Wunused-but-set-parameter
CXXFLAGS += -Wunused
CXXFLAGS += -Wstrict-aliasing
CXXFLAGS += -Wformat-security
CXXFLAGS += -Wno-unused-result
CXXFLAGS += -Wno-unused-variable
CXXFLAGS += -Wno-unused-but-set-variable
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-unwind-tables
CXXFLAGS += -fno-asynchronous-unwind-tables

CXX_STD = -std=c++20
CXX     = g++

OPTIMIZATION_LEVEL = -O3
ARCH_FLAGS = -march=native

SRC = src/frontend/compiler-hirola.cpp
BIN = bin/frontend/compiler-hirola

lexer:
	$(CXX) $(SRC) -o $(BIN) \
    $(CXXFLAGS) $(CXX_STD) $(ARCH_FLAGS) $(OPTIMIZATION_LEVEL)
