#ifndef PROCESS_FILE_H
#define PROCESS_FILE_H

#include "hashmap.h"

void processSegment(HashMap* map, char* fileChunk, size_t chunkSize, size_t spilloverSize);

#endif