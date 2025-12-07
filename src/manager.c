#include "mpi.h"

#include "config.h"

#include "manager.h"



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


// Run manager process
int runManager(FILE* file, int numWorkers, HashMap* resultsMap) {
    long long fileSize = getFileSize(file); // Get file size
    int curChunkOffset = 0;
    int activeWorkers = 0;

    // Create 2D buffer to hold worker's serialized results
    char** workerResults = malloc((numWorkers));
    
    int chunksProcessed = 0;
    
    // Send initial jobs to each worker
    for(int i = 1; i < numWorkers && curChunkOffset < fileSize; i++) {
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
            #ifdef DBG // DEBUG PRINT
                printf("\nStop signal sent to process %d", freeWorker);
            #endif
            
            int stop = -1;
            MPI_Send(&stop, 1, MPI_INT, freeWorker, TAG_STOP, MPI_COMM_WORLD);
            activeWorkers--;
        }
    }
    
    // Receive serialized title-words from workers
    for(int i = 0; i < numWorkers; i++) {
        MPI_Status status; // MPI message status
        int resultSize; // Size of worker's serialized results

        // Get size of next result
        MPI_Recv(&resultSize, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORDS_SIZE, MPI_COMM_WORLD, &status);

        // Allocate memory for serialized results
        workerResults[i] = malloc(resultSize);

        // Get data from worker
        MPI_Recv(&workerResults[i], resultSize, MPI_BYTE, status.MPI_SOURCE, TAG_WORDS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Add words to hashmap
    

    // Output results

    return 0;
}