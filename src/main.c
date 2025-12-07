#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "mpi.h"

#include "../include/worker.h"
#include "../include/manager.h"
#include "../include/config.h"


// include hashmap
#define HASHMAP_IMPLEMENTATION // Compiles implementation code
#include "../include/hashmap.h" // Compiles public api


// Error messages
#define ERR_MSG_ARGS "\nUsage: %s <path-to-text-file>" // Argument missing from program call
#define ERR_MSG_FNO "\nError opening '%s':\t%s" // Error opening file


typedef struct {
    long long offset;
    long long size;
} FileChunk;


static inline void hashMapMergeSum(HashMap *dst, HashMap *src) {
    // Create iterator for source map
    HashMapIterator it;
    iteratorInit(&it, src);

    // Holds source map entries
    const char *key;
    int value;

    // Iterate through source entries
    while(iteratorNext(&it, &key, &value)) {
        int existing;

        if(hashMapGet(dst, key, &existing)) // key exists, sum counts
            hashMapPut(dst, key, existing + value);
        else // key does not exist, copy entry from source
            hashMapPut(dst, key, value);
    }
}


void printWord(char* word, int* count) {
    printf("%s:\t%d\n", word, *count);
}


void printMap(HashMap* map) {
    HashMapIterator iter;
    iteratorInit(&iter, map);

    const char* word;
    int count;

    while(iteratorNext(&iter, &word, &count)) 
        printf("%s:\t%d\n", word, count);
}


int main(int argc, char* argv[]) {
    int procID, numProcs; // Total number of processes and rank
    double elapsedTime; // Tracks wall-clock execution time

    // Initialize MPI environment
    MPI_Init(&argc, &argv); // Initialize MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &procID); // Assign process identifier to id
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // Assign number of processes to p

    FILE* file = fopen(argv[1], "r"); // Attempt to open text file

    if(!file) { // Error opening file
        fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
        return ERR_CODE;
    }

    // Append empty space to file to handle first file chunk
    // Manager read first file word into map

    // Begin benchmark timing
    MPI_Barrier(MPI_COMM_WORLD); // Wait for processes
    elapsedTime = -MPI_Wtime(); // Start benchmark time

    if(!procID) { // Run manager process
        // Create hashmap to store manager's results
        HashMap titleWords;
        hashMapInit(&titleWords);

        runManager(file, numProcs - 1, &titleWords); // Run manager process
    } else { // Run worker process
        runWorker(file);
    }


    // Exit program gracefully
    MPI_Finalize();
    return SUCCESS_CODE;
}

