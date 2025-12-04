#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

//#include "mpi.h"

// include hashmap
#define HASHMAP_IMPLEMENTATION // Compiles implementation code
#include "../include/hashmap.h" // Compiles public api


// Error messages
#define ERR_MSG_ARGS "\nUsage: %s <path-to-text-file>" // Argument missing from program call
#define ERR_MSG_FNO "\nError opening '%s':\t%s" // Error opening file

// Return codes
#define ERR_CODE 1
#define SUCCESS_CODE 0

// #define READ_BUF_SIZE (32 * 1024 * 1024) // 32MB buffer size for fread


typedef struct {
    long long offset;
    long long size;
} FileChunk;


#define MAX_CHUNK_SIZE (4 * 4 * 1024) // Read file in 4 MB chunks
#define MAX_WORD_LEN 128

static inline bool advanceNextWord(char** curByte, size_t* bytesRead, size_t numBytes) {
    
    // Advanced to end of current word
    while(*bytesRead < numBytes && !isspace((unsigned char) **curByte)) {
        (*curByte)++; // Advanced character pointer
        (*bytesRead)++; // Increment num read
    }

    // Advanced to start of next word
    while(*bytesRead < numBytes && isspace((unsigned char) **curByte)) {
        (*curByte)++; // Advanced character pointer
        (*bytesRead)++; // Increment num read
    }

    return *bytesRead < numBytes; // Return true if more bytes to read
}

void incCount(const char* word, int* count) {
    (*count)++;
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

int processSegment(char* fileChunk, size_t chunkSize, int id) {
    // Create hashmap to store title-cased words
    HashMap map;
    hashMapInit(&map);
    
    // Process chars individually
    char* curByte = fileChunk; // Pointer to current position
    size_t bytesRead = 0; // Number of bytes read

    int i = 0; // DEBUG *******

    while(advanceNextWord(&curByte, &bytesRead, chunkSize)) {
        // DEBUG *******
        printf("Loop Entered (%zu / %zu Bytes Read)\n", bytesRead, chunkSize);
        fflush(stdout);


        // DEBUG *******
        i++;
        if(bytesRead > 10 || i > 10)
            break;

        char* wordStart = curByte; // Mark start of word
        size_t wordSize = 0; 

        // Check that first char is uppercase
        if(bytesRead < chunkSize && isupper((unsigned char) *curByte)) {
            // Advance file pointer
            wordSize++;
            bytesRead++;
            curByte++;

            // count subsequent lowercase letters
            while(bytesRead < chunkSize && islower((unsigned char) *curByte)) {
                // Advanced file pointer
                wordSize++;
                bytesRead++;
                curByte++;
            }

            // Title case if end of word is reached 
            if(bytesRead == chunkSize || isspace((unsigned char) *curByte)) {
                // Extract null-terminated word on stack
                char word[MAX_WORD_LEN]; // Static buffer
                memcpy(word, wordStart, wordSize); // Copy memory bytes
                word[wordSize] = '\0'; // Append null terminator

                // Add to hashmap
                if(!hashMapUpdate(&map, word, incCount))
                    hashMapPut(&map, wordStart, 1); 
            } 
        }
    }

    printf("\n\nSegment Processed:\n"); // DEBUG ********

    printMap(&map);
    return SUCCESS_CODE;
}



int runWorker(FILE* file, int id) {
    // Allocate chunk buffer
     char* fileChunk = malloc(sizeof(char) * MAX_CHUNK_SIZE);
    
    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    size_t chunkSize = fread(fileChunk, 1, MAX_CHUNK_SIZE, file);

    processSegment(fileChunk, chunkSize, id);

    return SUCCESS_CODE;
}

// Gets size of file in bytes as long long
// Resets file pointer position to start
static inline long long getFileSize(FILE* file) {
    if(fseeko(file, 0, SEEK_END)) // Go to end of file
        return -1;

    long long size = ftello(file); // Get current position byte offset
    
    if(fseeko(file, 0, SEEK_SET)) // Move back to start
        return -1;
    
    return size;
}

int main(int argc, char* argv[]) {
    int id = 0;

    FILE* file = fopen(argv[1], "rb"); // Attempt to open text file

    if(!file) { // Error opening file
        fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
        return ERR_CODE;
    }

    long long fileSize = getFileSize(file); // Get file size

    // DEBUG *******
    printf("%s (%lldB) opened successfully\n", argv[1], fileSize);

    size_t numChunks = (size_t) fileSize / MAX_CHUNK_SIZE;
    size_t lastChunkSize = (size_t) fileSize % MAX_CHUNK_SIZE;

    if(lastChunkSize)
        numChunks++;

    // DEBUG *******
    printf("Number of Chunks: %zu\tChunk Size: %d\tLast Chunk Size: %zu\n", numChunks, MAX_CHUNK_SIZE, lastChunkSize);

    // Allocate chunk buffer on heap
    char* fileChunk = malloc(sizeof(char) * MAX_CHUNK_SIZE);

    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    // Process all chunks
    for(size_t i = 0; i < numChunks; i++) {
        printf("Iteration %zu\n", i); // DEBUG *******
        fflush(stdout);

        // Read chunk into memory
        size_t chunkSize = fread(fileChunk, 1, MAX_CHUNK_SIZE, file);

        printf("\nChunk %zu read\tsize = %zu", i, chunkSize); // DEBUG ******
        fflush(stdout);

        if(chunkSize == 0) break; // EOF reached
        
        processSegment(fileChunk, chunkSize, id); // Process chunk
        //fseeko(file, (off_t) MAX_CHUNK_SIZE, SEEK_CUR); // Advance file pointer

        // DEBUG *******
        printf("\n\nSEGMENT PROCESSED\n\n");
    }

    return SUCCESS_CODE;
}

/*
int main(int argc, char* argv[]) {
    int id, p, totalValidIds; // Process id, number of MPI processes, and number of valid IDs
    double elapsedTime; // Tracks wall-clock execution time
    int processValidIds = 0; // Tracks valid IDs found per process

    // Initialize MPI environment
    MPI_Init(&argc, &argv); // Initialize MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // Assign process identifier to id
    MPI_Comm_size(MPI_COMM_WORLD, &p); // Assign number of processes to p
    createElementType(); // Create MPI datatype for title-cased words

    // Begin benchmark timing
    MPI_Barrier(MPI_COMM_WORLD); // Wait for processes
    elapsedTime = -MPI_Wtime(); // Start benchmark time

    FILE* file = fopen(argv[1], "r"); // Attempt to open text file

    if(!file) { // Error opening file
        fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
        return ERR_CODE;
    }


    if(!id) {
        if(argc < 2) { // Filepath argument missing
            fprintf(stderr, ERR_MSG_ARGS, argv[0]);
            return ERR_CODE;
        }

        runManagerProcess(p - 1, file);
    } else {
        runWorkerProcess(file);
    }


    // Convert hashmap to array of words

    // Sort array lexigraphically

    // Merge arrays

    // return merged array length

    // Wait for all processes

    // End benchmark time

    // Output array length as number of title-cased words

    return SUCCESS_CODE;
}*/