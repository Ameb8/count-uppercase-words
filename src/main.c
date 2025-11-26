#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "mpi.h"

// include hashmap
#define HASHMAP_IMPLEMENTATON // Compiles implementation code
#include "../include/hashmap.h" // Compiles public api


// Error messages
#define ERR_MSG_ARGS "\nUsage: %s <path-to-text-file>" // Argument missing from program call
#define ERR_MSG_FNO "\nError opening '%s':\t%s" // Error opening file

// Return codes
#define ERR_CODE 1
#define SUCCESS_CODE 0

#define MAX_WORD_LEN 1024
#define READ_BUF_SIZE (32 * 1024 * 1024) // 32MB buffer size for fread


// Gets size of file in bytes as long long
// Resets file pointer position to start
static inline long long getFileSize(FILE* file) {
    fseeko(file, 0, SEEK_END); // Go to end of file
    long long size = ftello(file); // Get current position byte offset
    fseeko(file, 0, SEEK_SET); // Move back to start
    
    return size;
}


// Advances file pointer to start of next word
// Returns the number of bytes advanced
long long advanceNextWord(FILE* file) {
    if(!file) // Validate file pointer
        return 0;

    int c; // Tracks current char
    long long advanced = 0; // Number of bytes advanced

    c = fgetc(file); // Inspect first character
    
    if(c == EOF) // End of file reached
        return 0;

    advanced++; // Increment bytes advanced

    if (!isspace(c)) { // Advance to end of current word
        while ((c = fgetc(file)) != EOF) {
            advanced++; // Increment bytes advanced
            
            if(isspace(c)) // End of word found
                break;
        }
    }

    // Skip whitespace to reach the start of the next word
    while((c = fgetc(file)) != EOF) { // Read until end of file
        advanced++; // Increment bytes advanced
        
        if(!isspace(c)) { // Start of next word found
            ungetc(c, file); // Unread first character in word
            advanced--; // Decrement bytes read to word start
            break; // Exit read loop
        }
    }

    return advanced; // Return number of bytes advanced
}


char* getTitleWord(FILE* file, long long* fileOffset) {

}

void incWord(const char* word, int* count) {
    (*count)++;
}


void processFileChunk(FILE* file, long long chunkSize) {
    // Create hashmap to store title-cased words
    HashMap map;
    hashMapInit(&map);

    long long bytesRead = 0;

    static char leftover[MAX_WORD_LEN] = {0};
    char* buffer = malloc(READ_BUF_SIZE + MAX_WORD_LEN); // extra space for leftover
    
    if(!buffer) // Error allocating buffer memory
        return;

    while(bytesRead < chunkSize) { // Read to end of buffer
        size_t toRead = READ_BUF_SIZE; // Max buffer size

        // Reduce buffer size if exceeds end of chunk
        if(chunkSize - bytesRead < (long long)READ_BUF_SIZE)
            toRead = (size_t)(chunkSize - bytesRead);

        // Copy leftover from previous chunk to beginning of buffer
        size_t leftoverLen = strlen(leftover);
        
        if(leftoverLen > 0) 
            memcpy(buffer, leftover, leftoverLen);

        size_t n = fread(buffer + leftoverLen, 1, toRead, file);
        if (n == 0) break; // EOF

        n += leftoverLen; // total valid bytes in buffer
        bytesRead += n - leftoverLen;

        size_t start = 0;
        for (size_t i = 0; i < n; ++i) {
            if (isspace((unsigned char)buffer[i])) {
                if (start < i) {
                    buffer[i] = '\0';
                    
                    // Increment count or initialize as zero
                    if(!hashMapApply(&map, buffer + start))
                        hashMapPut(&map, buffer + start, 0);
                }
                start = i + 1;
            }
        }

        // Handle leftover at end of buffer (partial word)
        if (start < n) {
            size_t leftoverSize = n - start;
            if (leftoverSize >= MAX_WORD_LEN) leftoverSize = MAX_WORD_LEN - 1;
            memcpy(leftover, buffer + start, leftoverSize);
            leftover[leftoverSize] = '\0';
        } else {
            leftover[0] = '\0';
        }

        // If less than chunkSize read, break (EOF)
        if (n - leftoverLen < toRead) break;
    }

    // After finishing chunk, process leftover if it’s a complete word
    if (strlen(leftover) > 0) {
        hashMapPut(map, leftover);
        leftover[0] = '\0';
    }

    free(buffer);






    char* buffer;
    long long chunkOffset = 0; // Tracks offset within file chunk

    while(chunkOffset < chunkSize) {
        // Read next full word
        const char* nextWord = getTitleWord(file, &chunkOffset);

        if(nextWord) { // Next word is title cased
            // Add to hashmap

            // Add 
        }

        chunkOffset += advancedNextWord(file); // Advanced to start of next word
    }
}


void runWorkerProcess() {
    // Send ready message

    // Wait for response

    // while msgResponse is file chunk
        // processFileChunk(fileChunk, chunkSize);
        // Send ready message
        // msgResponse <- response

}

void mergeEntry(const char* word, int count) {
    
}


void mergeMap(HashMap* main, HashMap* merge) {
    hashMapMap(&merge, mergeEntry);

}

void runManagerProcess(int numProcesses, FILE* file, long long chunkSize) {
    long long fileSize = getFileSize(file); // Get file size in Bytes
    long long nextChunkOffset = 0;

    // Post async message receives from workers

    while(nextChunkOffset < fileSize) {
        long long nextChunkSize = chunkSize; // Initialize next chunk size as maximum

        if(nextChunkOffset + chunkSize > fileSize) // Reduce chunk size if EOF will be exceeded
            nextChunkSize = fileSize - nextChunkOffset;

        // Respond to worker message with next file chunk

        nextChunkOffset += chunkSize; // Update next chunk offset
    }

    int processesCompleted = 0; // Tracks processes finished reading

    // Wait for all processes to complete
    while(processesCompleted < numProcesses - 1) {
        // Respond with EOF message

        
    }
    

}


int main(int argc, char* argv[]) {
    int id, p, totalValidIds; // Process id, number of MPI processes, and number of valid IDs
    double elapsedTime; // Tracks wall-clock execution time
    int processValidIds = 0; // Tracks valid IDs found per process

    // Initialize MPI environment
    MPI_Init(&argc, &argv); // Initialize MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // Assign process identifier to id
    MPI_Comm_size(MPI_COMM_WORLD, &p); // Assign number of processes to p

    // Begin benchmark timing
    MPI_Barrier(MPI_COMM_WORLD); // Wait for processes
    elapsedTime = -MPI_Wtime(); // Start benchmark time

    FILE* file = fopen(argv[1], "r"); // Attempt to open text file

    if(!file) { // Error opening file
        fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
        return ERR_CODE;
    }

    if(errCode )

    if(!id) {
        if(argc < 2) { // Filepath argument missing
            fprintf(stderr, ERR_MSG_ARGS, argv[0]);
            return ERR_CODE;
        }

        long long fileSize = getFileSize(file); // Get file size in Bytes
        
        // Get start/end offsets and size of section bieng read by process
        long long sectionSize = fileSize / (p - 1); // Get section size in Bytes
        long long fileOffset = sectionSize * id; // Get section start offset in Bytes
        long long endOffset = fileOffset + sectionSize; // Get section end offset in bytes
    }

    if(id) // Advance start offset to beggining of first full word
        fileOffset += advanceNextWord(file);

    while(fileOffset < endOffset) {
        // Read next full word
        const char* nextWord = getTitleWord(file, &fileOffset);

        if(nextWord) { // Next word is title cased
            // Add to hashmap

            // Add 
        }

        fileOffset += advancedNextWord(file); // Advanced to start of next word
    }

    // Convert hashmap to array of words

    // Sort array lexiographically

    // Merge arrays

    // return merged array length

    // Wait for all processes

    // End benchmark time

    // Output array length as number of title-cased words

    return SUCCESS_CODE;
}