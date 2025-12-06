#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "mpi.h"

// include hashmap
#define HASHMAP_IMPLEMENTATION // Compiles implementation code
#include "../include/hashmap.h" // Compiles public api


// Error messages
#define ERR_MSG_ARGS "\nUsage: %s <path-to-text-file>" // Argument missing from program call
#define ERR_MSG_FNO "\nError opening '%s':\t%s" // Error opening file

// Return codes
#define ERR_CODE 1
#define SUCCESS_CODE 0

// Read chunk and max word sizes
#define MAX_CHUNK_SIZE (4 * 4 * 1024) // Read file in 4 MB chunks
#define MAX_WORD_LEN 128

// MPI message tags
#define TAG_TASK 1
#define TAG_RESULT 2
#define TAG_STOP 3
#define TAG_WORDS 4
#define TAG_WORDS_SIZE 5
#define TAG_READY 6


typedef struct {
    long long offset;
    long long size;
} FileChunk;


typedef struct {
    char* data;
    int size;
} SerializedMap;


static inline SerializedMap serializeHashMap(HashMap* map) {
    // Create iterator for hashmap
    HashMapIterator it;
    iteratorInit(&it, map);

    // Stores next map entry
    const char* key;
    int value;

    // Stores data size variables
    int count = map->size;
    int totalSize = sizeof(int);

    // Count total data size in map
    while(iteratorNext(&it, &key, &value)) {
        int wordLen = strlen(key); // Get length of entry's key
        totalSize += sizeof(int) + wordLen + sizeof(int); // Compute size of entry
    }

    // Allocate memory for serialized map
    char* dataBuffer = malloc(totalSize); 
    char* curByte = dataBuffer; // Pointer to current location

    // Write number of entries to data
    memcpy(curByte, &count, sizeof(int));
    curByte += sizeof(int);

    iteratorInit(&it, map); // Reset hashmap
    
    // Write serialized data
    while(iteratorNext(&it, &key, &value)) {
        int wordLen = strlen(key); // Get length of next key

        // Copy length of next word into buffer
        memcpy(curByte, &wordLen, sizeof(int)); // Copy data
        curByte += sizeof(int); // Increment buffer pointer

        // Copy char* key to buffer
        memcpy(curByte, key, wordLen); // Copy data
        curByte += wordLen; // Increment buffer pointer
        
        // Copy int value to buffer
        memcpy(curByte, &value, sizeof(int)); // Copy data
        curByte += sizeof(int); // Increment buffer pointer
    }

    SerializedMap out = {dataBuffer, totalSize};
    return out;
}


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


void processSegment(HashMap* map, char* fileChunk, size_t chunkSize, size_t spilloverSize) {
    // Create hashmap to store title-cased words
    //HashMap map;
    //hashMapInit(&map);
    
    // Process chars individually
    char* curByte = fileChunk; // Pointer to current position
    size_t bytesRead = 0; // Number of bytes read

    // int i = 0; // DEBUG *******

    while(advanceNextWord(&curByte, &bytesRead, chunkSize)) {
        // DEBUG *******
        //printf("\n\nLoop Entered (%zu / %zu Bytes Read)\n", bytesRead, chunkSize);
        //printf("Starting chars: %c, %c, %c\n", curByte[0], curByte[1], curByte[2]);
        //fflush(stdout);
        // END DEBUG ***

        char* wordStart = curByte; // Mark start of word
        size_t wordSize = 0; 

        // Check that first char is uppercase
        if(bytesRead < chunkSize && isupper((unsigned char) *curByte)) {
            // Advance file pointer
            wordSize++;
            bytesRead++;
            curByte++;

            //printf("\nStart of title-word found: %c/n", *wordStart); // DEBUG *******

            // Count subsequent lowercase letters
            while(bytesRead < chunkSize && islower((unsigned char) *curByte)) {
                // Advanced file pointer
                wordSize++;
                bytesRead++;
                curByte++;
            }

            // Title case if end of word is reached 
            if(isspace((unsigned char) *curByte)) {
                // Extract null-terminated word on stack
                char word[MAX_WORD_LEN]; // Static buffer
                
                // Truncate words greater than MAX_WORD_LEN - 1
                if(wordSize >= MAX_WORD_LEN - 1)
                    wordSize = MAX_WORD_LEN - 1;

                memcpy(word, wordStart, wordSize); // Copy memory bytes
                word[wordSize] = '\0'; // Append null terminator

                //printf("\n\n%zu-len Title-Word Found:\t\"%s\"", wordSize, word); // DEBUG *******

                // Add to hashmap
                if(!hashMapUpdate(map, word, incCount))
                    hashMapPut(map, word, 1); 
            // Title-cased word overlaps end of file
            } else if(bytesRead == chunkSize) {
                // Advanced to end or non-lowercase of spillover word
                while(bytesRead < chunkSize + spilloverSize && islower((unsigned char) *curByte)) {
                    // Advanced file pointer
                    wordSize++;
                    bytesRead++;
                    curByte++;
                }

                // Check if spillover word is valid title-cased word
                if(bytesRead < chunkSize + spilloverSize && isspace((unsigned char) *curByte)) {
                    // Truncate words greater than MAX_WORD_LEN - 1
                    if(wordSize >= MAX_WORD_LEN - 1)
                        wordSize = MAX_WORD_LEN - 1;
                    
                    // Extract null-terminated word on stack
                    char word[MAX_WORD_LEN]; // Static buffer
                    memcpy(word, wordStart, wordSize); // Copy memory bytes
                    word[wordSize] = '\0'; // Append null terminator

                    // Add to hashmap
                    if(!hashMapUpdate(map, word, incCount))
                        hashMapPut(map, word, 1); 
                }
            }
        }
    }
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


int runWorker(FILE* file, int procID) {
    int chunkOffset;
    
    // Allocate chunk buffer on heap
    char* fileChunk = malloc(sizeof(char) * (MAX_CHUNK_SIZE + MAX_WORD_LEN));

    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    // Create hashmap to track title-cased words
    HashMap* map;


    while(1) { // Loop until STOP message received
        // Receive next task from manager
        MPI_Recv(&chunkOffset, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if(chunkOffset == -1) // No more chunks to process
            break;

        // Move file pointer to start of chunk
        fseeko(file, chunkOffset, SEEK_SET);

        // Read chunk into memory
        size_t chunkSize = fread(fileChunk, 1, MAX_CHUNK_SIZE + MAX_WORD_LEN, file);
        size_t spilloverSize = MAX_WORD_LEN;

        if(chunkSize < MAX_CHUNK_SIZE) // Partial file chunk
            spilloverSize = 0;
        // Full file chunk with partial spillover
        else if(chunkSize < MAX_CHUNK_SIZE + MAX_WORD_LEN)
            spilloverSize = chunkSize - MAX_CHUNK_SIZE;

        // Process file chunk
        //int numTitleWords = 0;
        processSegment(&map, fileChunk, chunkSize, spilloverSize);

        // Return title words to manager
        MPI_Send(NULL, 0, MPI_INT, 0, TAG_READY, MPI_COMM_WORLD);
    }

    // Serialize title-words to enable MPI communication
    SerializedMap titleWords = serializeHashMap(&map);

    // Send words to manager
    MPI_Send(&titleWords.size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // Send size
    MPI_Send(titleWords.data, titleWords.size, MPI_BYTE, 0, 0, MPI_COMM_WORLD); // Send data

    return SUCCESS_CODE;
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
        long long fileSize = getFileSize(file); // Get file size
        int curChunkOffset = 0;
        int activeWorkers = 0;
        int numTitleWords = 0;

        // Create 2D buffer to hold worker's serialized results
        char** workerResults = malloc((numProcs - 1));
        
        int chunksProcessed = 0;
        
        // Send initial jobs to each worker
        for(int i = 1; i < numProcs && curChunkOffset < fileSize; i++) {
            #ifdef DBG // DEBUG PRINT
                printf("\nInitial chunk at Byte-%d assigned to process %d", curChunkOffset, i);
            #endif

            MPI_Send(&curChunkOffset, 1, MPI_INT, i, TAG_TASK, MPI_COMM_WORLD);
            curChunkOffset += MAX_CHUNK_SIZE;
            activeWorkers++;
        }

        // Send chunks until none left
        while(activeWorkers > 0) {
            MPI_Status status; // Holds MPI message status

            // Block until worker is free
            MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, TAG_READY, MPI_COMM_WORLD, &status);
            chunksProcessed++; // Increment file chunks processed
            int freeWorker = status.MPI_SOURCE; // Get process ID of ready worker
            

            #ifdef DBG // DEBUG PRINT
                printf("\n%d/%d title-words read by process %d",result, numTitleWords, freeWorker);
            #endif

            if((long long) curChunkOffset < fileSize) { // Assign next chunk to free worker
                #ifdef DBG // DEBUG PRINT
                    printf("\nChunk at Byte-%d assigned to process %d", curChunkOffset, freeWorker);
                #endif

                MPI_Send(&curChunkOffset, 1, MPI_INT, freeWorker, TAG_TASK, MPI_COMM_WORLD);
                curChunkOffset += MAX_CHUNK_SIZE;
            } else { // Send stop signal to free worker
                #ifdef DBG // DEBUG PRINT
                    printf("\nStop signal sent to process %d", freeWorker);
                #endif
                
                int stop = -1;
                MPI_Send(&stop, 1, MPI_INT, freeWorker, TAG_STOP, MPI_COMM_WORLD);
                activeWorkers--;
            }
        }
        
        // Receive serialized title-words from workers
        for(int i = 0; i < numProcs - 1; i++) {
            MPI_Status status; // MPI message status
            int resultSize; // Size of worker's serialized results

            // Get size of next result
            MPI_Recv(&resultSize, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORDS_SIZE, MPI_COMM_WORLD, &status);

            // Allocate memory for serialized results
            workerResults[i] = malloc(resultSize);

            // Get data from worker
            MPI_Recv(&workerResults[i], resultSize, MPI_BYTE, status.MPI_SOURCE, TAG_WORDS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Create hashmap to store all file's words
        HashMap titleWords;
        hashMapInit(&titleWords);

        // Add words to hashmap

        // Output results

    } else { // Run worker process
        runWorker(file, procID);
    }


    // Exit program gracefully
    MPI_Finalize();
    return SUCCESS_CODE;
}

