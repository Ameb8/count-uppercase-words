#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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


typedef struct {
    long long offset;
    long long size;
} FileChunk;

/*
typedef struct {
    int count;
    char word[MAX_WORD_LEN];
} TitleWord;


MPI_Datatype MPI_Element;


static inline void createElementType() {
    int lengths[2] = {1, MAX_WORD_LEN};
    MPI_Aint offsets[2];
    offsets[0] = offsetof(TitleWord, count);
    offsets[1] = offsetof(TitleWord, word);

    MPI_Datatype types[2] = {MPI_INT, MPI_CHAR};

    MPI_Type_create_struct(2, lengths, offsets, types, &MPI_Element);
    MPI_Type_commit(&MPI_Element);
}



static inline void getSendBuffer(HashMap* titleWords) {
    // Create array to hold data
    TitleWord* sendBuffer;
    
    // Create iterator for hashmap
    HashMapIterator mapIter;
    iteratorInit(&mapIter, titleWords);

    for(int i = 0; i < sendBufferSize; i++) {
        TitleWord word;

        // Add word to send buffer
        if(!iteratorNext(&mapIter, &sendBuffer[i].word, &sendBuffer[i].count))
            break;
    }
}


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

/*
void runWorkerProcess() {
    // Send ready message

    // Wait for response

    // while msgResponse is file chunk
        // processFileChunk(fileChunk, chunkSize);
        // Send ready message
        // msgResponse <- response

}
*/

/*

void mergeMap(HashMap* main, HashMap* merge) {
    // Initialize iterator for merging map
    HashMapIterator iter;
    iteratorInit(&iter, merge);

    // Initialize vars to hold next word count in merging map
    const char* nextWord;
    int count;

    // Nested function 
    void incrementBy(const char *key, int *value) {
        *value += count;
    }

    while(iteratorNext(&iter, &nextWord, &count)) {
        // Increment main hashmap word count if contains key
        if(!hashMapUpdate(main, nextWord, incrementBy))
            hashMapPut(main, nextWord, count); // Add key to main hashmap
    }

    


}




// Gets size of file in bytes as long long
// Resets file pointer position to start
static inline long long getFileSize(FILE* file) {
    fseeko(file, 0, SEEK_END); // Go to end of file
    long long size = ftello(file); // Get current position byte offset
    fseeko(file, 0, SEEK_SET); // Move back to start
    
    return size;
}
*/
/*
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
*/

 /*

void runManagerProcess(int numWorkers, FILE* file) {
    long long fileSize = getFileSize(file); // Get file size in Bytes
    
    // Get start/end offsets and size of section being read by process
    long long sectionSize = fileSize / numWorkers; // Get section size in Bytes
    long long sectionRmd = fileSize % numWorkers; // Get remainder
    //long long fileOffset = sectionSize * id; // Get section start offset in Bytes
    //long long endOffset = fileOffset + sectionSize; // Get section end offset in bytes

    for(int i = 0; i < numWorkers; i++) { // Send file size and start offset to all workers
        long long chunkSize = sectionSize; // Set default chunk size
        
        if(i == numWorkers - 1) // Give remainder to last worker
            chunkSize += sectionRmd;

        // Prepare file chunk to send to worker
        long long fileChunk[2] = {sectionSize * i, chunkSize};
        
        // Send file chunk to worker
        MPI_Send(fileChunk, 2, MPI_LONG_LONG, i + 1, 0, MPI_COMM_WORLD);
    }
}


int runWorkerProcess(FILE* file) {
    // Get file chunk from manager process
    long long fileChunk[2];
    MPI_Recv(fileChunk, 2, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // Assign chunk size to variables
    long long currentByte = fileChunk[0];
    long long chunkEnd = fileChunk[1] + currentByte;

    // Move file pointer to begining of chunk
    if(!fseek(file, currentByte, SEEK_SET)) {
        // Error occurred in fseek()
        printf("\nError occurred seeking in file");
        fclose(file);
        return ERR_CODE;
    }

    // Advanced to first full word in chunk
    if(currentByte) // Skip for first chunk
        currentByte += advanceNextWord(file);

    // Create hashmap to store title-cased words
    HashMap map;
    hashMapInit(&map);

    // Process every word in chunk
    while(currentByte < chunkEnd) {
        char* nextWord[MAX_WORD_LEN];
        char isUpperCase = 0;

        // Read next word
        currentByte += isTitleCased(file, nextWord, &isUpperCase);

        if(isUpperCase) { // Add word to hashmap if title-cased
            // Increment count or initialize as one
            if(!hashMapApply(&map, nextWord, incWord))
                hashMapPut(&map, nextWord, 1);
        }
    }

    // Send title-cased words and count to root process
    MPI_Send();

    
}

*/

/*
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
*/

#define MAX_CHUNK_SIZE 4 * 4 * 1024 // Read file in 4 MB chunks
#define MAX_WORD_LEN 256

static inline bool advanceNextWord(char** curByte, size_t* bytesRead, size_t numBytes) {
    
    // Advanced to end of current word
    while(*bytesRead < numBytes && isalpha((unsigned char) **curByte)) {
        (*curByte)++; // Advanced character pointer
        (*bytesRead)++; // Increment num read
    }

    // Advanced to start of next word
    while(*bytesRead < numBytes && isspace((unsigned char) **curByte)) {
        (*curByte)++; // Advanced character pointer
        (*bytesRead)++; // Increment num read
    }

    return bytesRead < numBytes; // Return true if more bytes to read
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

    char* word;
    int count;

    while(iteratorNext(&iter, &word, &count)) 
        printf("%s:\t%d\n", word, count);
}

int processSegment(char** fileChunk, size_t chunkSize, int id) {
    // Create hashmap to store title-cased words
    HashMap map;
    hashMapInit(&map);
    
    // Process chars individually
    char* curByte = *fileChunk; // Pointer to current position
    size_t bytesRead = 0; // Number of bytes read

    while(advanceNextWord(&curByte, &bytesRead, chunkSize)) {
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

    printMap(&map);
    
}



int runWorker(FILE* file, int id) {
    // Allocate chunk buffer
     char* fileChunk = malloc(sizeof(char) * MAX_CHUNK_SIZE);
    
    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    size_t chunkSize = fread(fileChunk, 1, MAX_CHUNK_SIZE, file);

    processSegment(&fileChunk, chunkSize, id);

}

// Gets size of file in bytes as long long
// Resets file pointer position to start
static inline long long getFileSize(FILE* file) {
    fseeko(file, 0, SEEK_END); // Go to end of file
    long long size = ftello(file); // Get current position byte offset
    fseeko(file, 0, SEEK_SET); // Move back to start
    
    return size;
}

int main(int argc, char* argv[]) {
    int id = 0;

    FILE* file = fopen(argv[1], "r"); // Attempt to open text file

    if(!file) { // Error opening file
        fprintf(stderr, ERR_MSG_FNO, argv[1], strerror(errno));
        return ERR_CODE;
    }

    long long fileSize = getFileSize(file); // Get file size
    
    size_t numChunks = fileSize / MAX_CHUNK_SIZE;
    size_t lastChunkSize = fileSize % MAX_CHUNK_SIZE;

    if(!lastChunkSize)
        numChunks++;

    // Allocate chunk buffer on heap
    char* fileChunk = malloc(sizeof(char) * MAX_CHUNK_SIZE);

    if(!fileChunk) // Memory allocation failed
        return ERR_CODE;

    // Process all chunks
    for(int i = 0; i < numChunks; i++) {
        // Read chunk into memory
        size_t chunkSize = fread(fileChunk, 1, MAX_CHUNK_SIZE, file);
        
        processSegment(&fileChunk, chunkSize, id); // Process chunk
        fseeko(file, (off_t) MAX_CHUNK_SIZE, SEEK_CUR); // Advance file pointer
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