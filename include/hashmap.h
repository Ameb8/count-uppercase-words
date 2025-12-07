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
    int size;
} HashMap;


typedef struct {
    HashMap *map; // Hashmap being iterated
    int bucketIndex; // Current bucket
    HashMapEntry *entry; // Current entry in the bucket
} HashMapIterator;


// Public HashMap API
void hashMapInit(HashMap *m);
void hashMapPut(HashMap *m, const char *key, int value);
char hashMapUpdate(HashMap *m, const char *key, void (*fn)(const char*, int*));
int hashMapGet(HashMap *m, const char *key, int *outValue);
void hashMapMap(HashMap *m, void (*fn)(const char *, int));
void hashMapFree(HashMap *m);

// Public Iterator API
void iteratorInit(HashMapIterator *it, HashMap *map);
char iteratorNext(HashMapIterator *it, const char **key, int *value);


#ifdef HASHMAP_IMPLEMENTATION


// djb2 hashing function
unsigned long hash(const char *str) {
    unsigned long hash = 5381; // Initial hash value
    int c; // Stores next char in key
    
    while((c = *str++)) // Apply hash function
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}


// Initialize empty hashmap
void hashMapInit(HashMap *m) {
    // Initialize all buckets to null
    for(int i = 0; i < HM_TABLE_SIZE; i++)
        m->buckets[i] = NULL;

    m->usedCount = 0;
    m->size = 0;
}


// Insert new value or update existing
void hashMapPut(HashMap *m, const char *key, int value) {
    unsigned long h = hash(key) % HM_TABLE_SIZE;
    HashMapEntry *e = m->buckets[h];

    while(e) { // Update existing key
        if(strcmp(e->key, key) == 0) {
            e->value = value;
            return;
        }
        e = e->next;
    }

    // Add bucket to used buckets if first entry
    if(m->buckets[h] == NULL) 
        m->usedBuckets[m->usedCount++] = h;

    // Create new entry
    e = (HashMapEntry *)malloc(sizeof(HashMapEntry));
    e->key = strdup(key);
    e->value = value;
    e->next = m->buckets[h];
    m->buckets[h] = e;
    m->size++;
}


char hashMapUpdate(HashMap *m, const char *key, void (*fn)(const char*, int*)) {
    unsigned long h = hash(key) % HM_TABLE_SIZE;
    HashMapEntry *e = m->buckets[h];

    while(e) { // 
        if(strcmp(e->key, key) == 0) { // Check if correct key
            // Apply fn to entry
            fn(e->key, &e->value);
            return 1;
        }

        e = e->next; // Advance to next entry in bucket
    }

    return 0;
}


// Retrieve value associated with key in hashmap
int hashMapGet(HashMap *m, const char *key, int *outValue) {
    unsigned long h = hash(key) % HM_TABLE_SIZE;
    HashMapEntry *e = m->buckets[h];

    while(e) { // Iterate through bucket entries
        if(strcmp(e->key, key) == 0) { // Check if correct key
            // Key found
            *outValue = e->value;
            return 1;
        }

        e = e->next; 
    }

    return 0; // Key not found
}


void hashMapMap(HashMap *m, void (*fn)(const char *, int)) {
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
void hashMapFree(HashMap *m) {
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


// Initialize hashmap iterator
void iteratorInit(HashMapIterator *it, HashMap *map) {
    it->map = map;
    it->bucketIndex = 0;
    it->entry = NULL;

    // Find first non-empty bucket
    while(it->bucketIndex < map->usedCount) {
        int b = map->usedBuckets[it->bucketIndex];

        if(map->buckets[b]) {
            it->entry = map->buckets[b];
            break;
        }

        it->bucketIndex++;
    }
}


// Advance HashMap iterator and retrieve key/value
char iteratorNext(HashMapIterator *it, const char **key, int *value) {
    if(!it->entry) { // No more entries
        *key = NULL;
        return 0;
    } else { // Process next entry
        *key = it->entry->key;
        *value = it->entry->value;

        // Move to next entry in current bucket
        if(it->entry->next) { // Move to next entry in bucket
            it->entry = it->entry->next;
        } else { // Move to next used bucket
            it->bucketIndex++;
            it->entry = NULL;

            while(it->bucketIndex < it->map->usedCount) {
                int b = it->map->usedBuckets[it->bucketIndex++]; // Get next used bucket index

                if(it->map->buckets[b]) {
                    it->entry = it->map->buckets[b];
                    break;
                }
            }
        }
    }

    return 1;
}


#endif // HASHMAP_IMPLEMENTATION
#endif // HASHMAP_H