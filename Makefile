CC       = mpicc
INCLUDES = -Iinclude
SRC      = $(wildcard src/*.c)

# Object directories per build
OBJ_DIR      = obj
OBJ_DIR_DBG  = obj_dbg
OBJ_DIR_OPT  = obj_opt

# Compiler flags
CFLAGS       = -Wall -Wextra -O2 $(INCLUDES)
CFLAGS_DBG   = -Wall -Wextra -g -DDBG $(INCLUDES)
CFLAGS_OPT   = -Wall -Wextra -O3 -march=native -funroll-loops -flto $(INCLUDES)

# Binaries
BIN       = bin/count
BIN_DBG   = bin/count_dbg
BIN_OPT   = bin/count_opt

# Default target
all: $(BIN)

# Pattern rules to compile each .c to .o for each build
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR_DBG)/%.o: src/%.c | $(OBJ_DIR_DBG)
	$(CC) -c $< -o $@ $(CFLAGS_DBG)

$(OBJ_DIR_OPT)/%.o: src/%.c | $(OBJ_DIR_OPT)
	$(CC) -c $< -o $@ $(CFLAGS_OPT)

# Create object directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR_DBG):
	mkdir -p $(OBJ_DIR_DBG)

$(OBJ_DIR_OPT):
	mkdir -p $(OBJ_DIR_OPT)

# Normal build
OBJ       = $(SRC:src/%.c=$(OBJ_DIR)/%.o)
$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@

# Debug build
OBJ_DBG   = $(SRC:src/%.c=$(OBJ_DIR_DBG)/%.o)
dbg: $(BIN_DBG)

$(BIN_DBG): $(OBJ_DBG)
	$(CC) $(OBJ_DBG) $(CFLAGS_DBG) -o $@

# Optimized build
OBJ_OPT   = $(SRC:src/%.c=$(OBJ_DIR_OPT)/%.o)
opt: $(BIN_OPT)

$(BIN_OPT): $(OBJ_OPT)
	$(CC) $(OBJ_OPT) -o $@

# Clean all builds
clean:
	rm -rf $(OBJ_DIR) $(OBJ_DIR_DBG) $(OBJ_DIR_OPT) $(BIN) $(BIN_DBG) $(BIN_OPT)
