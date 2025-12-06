CC = mpicc # Set compiler

SRC = $(wildcard src/*.c) # Source .c files in src/

# Output binaries
BIN = bin/count
BIN_DBG = bin/count_dbg
BIN_OPT = bin/count_opt

# Compiler flags
CFLAGS = -Wall -Wextra -O2 # Standard build
CFLAGS_DBG = $(CFLAGS) -g -fsanitize=address -DDBG # Debug build
CFLAGS_OPT = -Wall -Wextra -march=native -funroll-loops -flto # Optimized build

all: $(BIN) # Default target

# Normal build
$(BIN): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS)

# Debug build
$(BIN_DBG): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS_DBG)

# Optimized build
$(BIN_OPT): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS_OPT)

# Aliases for convenience
dbg: $(BIN_DBG) # Alias to build debug target
opt: $(BIN_OPT) # Alias to build optimized target

# Clean
clean:
	rm -f $(BIN) $(BIN_DBG)
