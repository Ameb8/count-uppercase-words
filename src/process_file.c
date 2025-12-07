#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "../include/config.h"

#include "../include/process_file.h"


void incCount(const char* word, int* count) {
    (*count)++;
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


void processSegment(HashMap* map, char* fileChunk, size_t chunkSize, size_t spilloverSize) {
    // Create hashmap to store title-cased words
    //HashMap map;
    //hashMapInit(&map);
    
    // Process chars individually
    char* curByte = fileChunk; // Pointer to current position
    size_t bytesRead = 0; // Number of bytes read

    // int i = 0; // DEBUG *******

    // Loop until full chunk processed
    while(advanceNextWord(&curByte, &bytesRead, chunkSize)) {
        char* wordStart = curByte; // Mark start of word
        size_t wordSize = 0; 

        // Check that first char is uppercase
        if(bytesRead < chunkSize && isupper((unsigned char) *curByte)) {
            // Advance file pointer
            wordSize++;
            bytesRead++;
            curByte++;

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
