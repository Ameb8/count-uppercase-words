#!/bin/bash


# Set default executable target
target=./bin/count

# Set target for alternate build
if [ -n "$3" ]; then
    target=./bin/count_$3
fi


make $3 > /dev/null # Recompile if necessary
mpirun -np $1 -oversubscribe $target $2 # Run program
