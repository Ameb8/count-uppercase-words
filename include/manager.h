#ifndef MANAGER_H
#define MANAGER_H

#include <stdio.h>

#include "hashmap.h"

int runManager(FILE* file, int numWorkers, HashMap* resultsMap);

#endif