#ifndef CONFIG_H
#define CONFIG_H

// Program return codes
#define ERR_CODE 1
#define SUCCESS_CODE 0

// Read chunk and max word sizes
#define MAX_CHUNK_SIZE (4 * 4 * 1024) // Read file in 16 KB chunks
#define MAX_WORD_LEN 128

// MPI message tags
#define TAG_TASK 1
#define TAG_RESULT 2
#define TAG_STOP 3
#define TAG_WORDS 4
#define TAG_WORDS_SIZE 5
#define TAG_READY 6

#endif
#define MAX_CHUNK_SIZE 16384
