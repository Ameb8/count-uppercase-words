#include <stdio.h>
#include "mpi.h"

#include "../include/process_file.h"
#include "../include/config.h"

#include "../include/worker.h"


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


int runWorker(FILE* file) {
    int chunkOffset;
    
    // Allocate chunk buffer on heap
    char* fileChunk = malloc(sizeof(char) * (MAX_CHUNK_SIZE + MAX_WORD_LEN));

    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    // Create hashmap to track title-cased words
    HashMap map;
    hashMapInit(&map);


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
