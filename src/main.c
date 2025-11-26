#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "mpi.h"


// Error messages
#define ERR_MSG_ARGS "\nUsage: %s <path-to-text-file>" // Argument missing from program call
#define ERR_MSG_FNO "\nError opening '%s':\t%s" // Error opening file

// Return codes
#define ERR_CODE 1
#define SUCCESS_CODE 0

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


void processFileChunk(FILE* file, long long chunkSize) {
    // Create hashmap to store title-cased words

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

void runManagerProcess(int numProcesses, FILE* file, long long chunkSize) {
    long long fileSize = getFileSize(file); // Get file size in Bytes
    long long nextChunkOffset = 0;

    while(nextChunkOffset < fileSize) {
        
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

    if(!id) {
        if(argc < 2) { // Filepath argument missing
            fprintf(stderr, ERR_MSG_ARGS, argv[0]);
            return ERR_CODE;
        }

        FILE* file = fopen(argv[1], "r"); // Attempt to open text file

        if(!file) { // Error opening file
            fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
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