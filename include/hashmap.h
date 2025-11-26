#ifndef HASHMAP_H
#define HASHMAP_H


#include <stdlib.h>
#include <string.h>


// Set overridable table size
#ifndef HM_TABLE_SIZE
#define HM_TABLE_SIZE 1024  
#endif


// Key-Value pair
typedef struct HashMapEntry {
    char *key; // Entry key
    int value; // Entry value
    struct HashMapEntry *next; // Pointer to next entry with hash-value
} HashMapEntry;


// Hashmap struct
typedef struct {
    HashMapEntry *buckets[HM_TABLE_SIZE]; // Array-based hashmap
    int usedBuckets[HM_TABLE_SIZE]; // Tracks buckets with values
    int usedCount; // Tracks number of used buckets
} HashMap;


// djb2 hashing function
static inline unsigned long hash(const char *str) {
    unsigned long hash = 5381; // Initial hash value
    int c; // Stores next char in key
    
    while((c = *str++)) // Apply hash function
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}


// Initialize empty hashmap
static inline void hashmapInit(HashMap *m) {
    // Initialize all buckets to null
    for (int i = 0; i < HM_TABLE_SIZE; i++)
        m->buckets[i] = NULL;

    m->usedCount = 0;
}


// Insert new value or update existing
static inline void hashmapPut(HashMap *m, const char *key, int value) {
    unsigned long h = hash(key) % HM_TABLE_SIZE;
    HashMapEntry *e = m->buckets[h];

    while(e) { // Update existing key
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return;
        }
        e = e->next;
    }

    // Add bucket to used buckets if first entry
    if (m->buckets[h] == NULL) 
        m->usedBuckets[m->usedCount++] = h;

    // Create new entry
    e = (HashMapEntry *)malloc(sizeof(HashMapEntry));
    e->key = strdup(key);
    e->value = value;
    e->next = m->buckets[h];
    m->buckets[h] = e;
}


// Retrieve value associated with key in hashmap
static inline int hashmapGet(HashMap *m, const char *key, int *outValue) {
    unsigned long h = hash(key) % HM_TABLE_SIZE;
    HashMapEntry *e = m->buckets[h];

    while(e) { // Iterate through bucket entries
        if (strcmp(e->key, key) == 0) { // Check if correct key
            // Key found
            *outValue = e->value;
            return 1;
        }
        e = e->next;
    }

    return 0; // Key not found
}



static inline void hashmapMap(HashMap *m, void (*fn)(const char *, int)) {
    // Iterate through used buckets
    for(int i = 0; i < m->usedCount; i++) {
        int bucket = m->usedBuckets[i];
        HashMapEntry *e = m->buckets[bucket];

        while(e) { // Apply function to bucket entries
            fn(e->key, e->value);
            e = e->next;
        }
    }
}


// Deallocate hashmap memory
static inline void hashmapFree(HashMap *m) {
    // Iterate through used buckets
    for(int i = 0; i < m->usedCount; i++) {
        int bucket = m->usedBuckets[i];
        HashMapEntry *e = m->buckets[bucket];

        while(e) { // Iterate bucket entries
            HashMapEntry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
}


#endif /* HASHMAP_H */
