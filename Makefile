CC         = mpicc
INCLUDES   = -Iinclude
SRC        = $(wildcard src/*.c)
OBJ        = $(SRC:src/%.c=obj/%.o)

CFLAGS     = -Wall -Wextra -O2 $(INCLUDES)
CFLAGS_DBG = -Wall -Wextra -g -fsanitize=address -DDBG $(INCLUDES)
CFLAGS_OPT = -Wall -Wextra -march=native -funroll-loops -flto $(INCLUDES)

BIN       = bin/count
BIN_DBG   = bin/count_dbg
BIN_OPT   = bin/count_opt

# Default target
all: $(BIN)

# Create obj directory
obj:
	mkdir -p obj

# Pattern rule: compile each .c to .o
obj/%.o: src/%.c | obj
	$(CC) -c $< -o $@ $(CFLAGS)

# Normal build
$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@

# Debug build
dbg: CFLAGS = $(CFLAGS_DBG)
dbg: clean $(BIN_DBG)

$(BIN_DBG): $(OBJ)
	$(CC) $(OBJ) -o $@

# Optimized build
opt: CFLAGS = $(CFLAGS_OPT)
opt: clean $(BIN_OPT)

$(BIN_OPT): $(OBJ)
	$(CC) $(OBJ) -o $@

clean:
	rm -rf obj $(BIN) $(BIN_DBG) $(BIN_OPT)
