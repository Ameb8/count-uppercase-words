#include <stdlib.h>

#include "mpi.h"

#include "config.h"

#include "manager.h"


static int mergingCount;


static void mergeUpdate(const char* word, int* count) {
    *count += mergingCount;
}

/*
static inline void mergeToMap(HashMap* map, char* data, int dataSize) {
    if(dataSize < sizeof(int)) { // Ensure data contains title words
        #ifdef DBG // DEBUG PRINT
            printf("\nNo data to aggregate (%d)", dataSize);
            fflush(stdout);
        #endif
        
        return;
    }
    
    int bytesRead = 0;

    // Ignore entry count
    bytesRead += sizeof(int);
    data += sizeof(int);

    while(bytesRead < dataSize) {
        // Get length of next word
        int wordLen; // Length of next word
        memcpy(&wordLen, data, sizeof(int)); // Read word length

        data += sizeof(int); // Increment data pointer
        bytesRead += sizeof(int); // Increment bytes read

        const char* nextWord = data; // Store start of next word
        data += wordLen; // Increment data pointer to word's count
        bytesRead += wordLen; // Increment bytes read
        memcpy(&mergingCount, data, sizeof(int)); // Read word count
        bytesRead += sizeof(int);

        // Merge word into hashmap
        if(!hashMapUpdate(map, nextWord, mergeUpdate))
            hashMapPut(map, nextWord, mergingCount);
    }
}
*/


static inline void mergeToMap(HashMap* map, char* data, int dataSize) {
    if (dataSize < sizeof(int)) return; // empty buffer

    int bytesRead = 0;

    // Read entry count
    int entryCount;
    memcpy(&entryCount, data, sizeof(int));
    data += sizeof(int);
    bytesRead += sizeof(int);

    for (int i = 0; i < entryCount; i++) {
        if (bytesRead + sizeof(int) > dataSize) break; // sanity check

        int wordLen;
        memcpy(&wordLen, data, sizeof(int));
        data += sizeof(int);
        bytesRead += sizeof(int);

        if (bytesRead + wordLen + sizeof(int) > dataSize) break; // sanity check

        const char* nextWord = data;
        data += wordLen; // move past the key
        bytesRead += wordLen;

        int value;
        memcpy(&value, data, sizeof(int)); // read value here
        data += sizeof(int); // then move pointer
        bytesRead += sizeof(int);

        mergingCount = value; // store value for mergeUpdate

        if (!hashMapUpdate(map, nextWord, mergeUpdate))
            hashMapPut(map, nextWord, mergingCount);
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


// Prints program results
void printResults(HashMap* results) {
    // Initialize hashmap iterator
    HashMapIterator it;
    iteratorInit(&it, results);

    unsigned long long totalWords = 0;

    const char* nextWord;
    int nextCount;

    // Iterate results map entries
    while(iteratorNext(&it, &nextWord, &nextCount)) {
        printf("\n%s: %d", nextWord, nextCount); // Display word/count
        totalWords += (unsigned long long) nextCount; // Increment total words
    }

    // Print total words
    printf("\n\nUnique Title-Cased Words:\t\t%d", results->size);
    printf("\nTotal Title-Cased Words:\t\t%llu", totalWords);
}


// Run manager process
int runManager(FILE* file, int numWorkers, HashMap* resultsMap) {
    long long fileSize = getFileSize(file); // Get file size
    int curChunkOffset = 0;
    int activeWorkers = 0;

    // Create 2D buffer to hold worker's serialized results
    char** workerResults = malloc(numWorkers * sizeof(char*));
    int* resultSizes = malloc(numWorkers * sizeof(int)); // Number of bytes in worker's result
    
    int chunksProcessed = 0;
    
    // Send initial jobs to each worker
    for(int i = 1; i <= numWorkers && curChunkOffset < fileSize; i++) {
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
            printf("\nChunks processed by process %d", freeWorker);
        #endif

        if((long long) curChunkOffset < fileSize) { // Assign next chunk to free worker
            #ifdef DBG // DEBUG PRINT
                printf("\nChunk at Byte-%d assigned to process %d", curChunkOffset, freeWorker);
            #endif

            MPI_Send(&curChunkOffset, 1, MPI_INT, freeWorker, TAG_TASK, MPI_COMM_WORLD);
            curChunkOffset += MAX_CHUNK_SIZE;
        } else { // Send stop signal to free worker            
            int stop = -1;
            MPI_Send(&stop, 1, MPI_INT, freeWorker, TAG_STOP, MPI_COMM_WORLD);
            --activeWorkers;

            #ifdef DBG // DEBUG PRINT
                printf("\nStop signal sent to process %d (active workers: %d)", freeWorker, activeWorkers);
            #endif
        }
    }

    #ifdef DBG // DEBUG PRINT
        printf("\nAll file chunks have been processed");
        fflush(stdout);
    #endif
    
    // Receive serialized title-words from workers
    for(int i = 0; i < numWorkers; i++) {
        MPI_Status status; // MPI message status

        // Get size of next result
        MPI_Recv(&resultSizes[i], 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORDS_SIZE, MPI_COMM_WORLD, &status);

        #ifdef DBG // DEBUG PRINT
            printf("\nSerialized words size %d received from process %d", resultSizes[i], status.MPI_SOURCE);
            fflush(stdout);
        #endif

        // Allocate memory for serialized results
        workerResults[i] = malloc(resultSizes[i]);

        // Get data from worker
        MPI_Recv(workerResults[i], resultSizes[i], MPI_BYTE, status.MPI_SOURCE, TAG_WORDS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        #ifdef DBG // DEBUG PRINT
            printf("\nSerialized words data received from process %d (%d / %d)", status.MPI_SOURCE, i, numWorkers);
            fflush(stdout);
        #endif
    }

    #ifdef DBG // DEBUG PRINT
        printf("\nAll serialized data received");
        fflush(stdout);
    #endif

    for(int i = 0; i < numWorkers; i++) { // Add words to hashmap
        mergeToMap(resultsMap, workerResults[i], resultSizes[i]);
        free(workerResults[i]);
    }
 
    // Free allocated memory
    free(workerResults);
    free(resultSizes);

    // Output results
    printResults(resultsMap);

    return 0;
}