#ifndef HASHMAP_H
#define HASHMAP_H


#include <stdlib.h>
#include <string.h>


// Set overridable table size
#ifndef HM_TABLE_SIZE
#define HM_TABLE_SIZE 1024  
#endif


// Key-Value pair
typedef struct hm_entry {
    char *key;
    int value;
    struct hm_entry *next;
} hm_entry;


// Hashmap struct
typedef struct {
    hm_entry *buckets[HM_TABLE_SIZE];
} hm_map;


// djb2 hashing function
static inline unsigned long hm_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


// Initialize empty hashmap
static inline void hm_init(hm_map *m) {
    for (int i = 0; i < HM_TABLE_SIZE; i++)
        m->buckets[i] = NULL;
}


// Insert new value or update existing
static inline void hm_put(hm_map *m, const char *key, int value) {
    unsigned long h = hm_hash(key) % HM_TABLE_SIZE;
    hm_entry *e = m->buckets[h];

    while(e) { // Update existing key
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return;
        }
        e = e->next;
    }

    // Create new entry
    e = (hm_entry *)malloc(sizeof(hm_entry));
    e->key = strdup(key);
    e->value = value;
    e->next = m->buckets[h];
    m->buckets[h] = e;
}


// Retrieve value associated with key in hashmap
static inline int hm_get(hm_map *m, const char *key, int *out_value) {
    unsigned long h = hm_hash(key) % HM_TABLE_SIZE;
    hm_entry *e = m->buckets[h];

    while(e) { // Iterate through values at hash index
        if (strcmp(e->key, key) == 0) { // Check if correct key
            // Key found
            *out_value = e->value;
            return 1;
        }
        e = e->next;
    }

    return 0; // Key not found
}


// Deallocate hashmap memory
static inline void hm_free(hm_map *m) {
    for (int i = 0; i < HM_TABLE_SIZE; i++) {
        hm_entry *e = m->buckets[i];
        while (e) {
            hm_entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
}


#endif /* HASHMAP_H */
