#!/bin/bash


# Set default executable target
target=./bin/count

# Set target for alternate build
if [ -n "$2" ]; then
    target=./bin/count_$2
fi


make $2 # Recompile if necessary
mpirun -np $1 -oversubscribe $target test_files/big.txt # Run program
