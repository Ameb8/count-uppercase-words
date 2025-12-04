#include <stdio.h>
#include <stdlib.h>

// Gets size of file in bytes as long long
// Resets file pointer position to start
static inline long long getFileSize(FILE* file) {
    if(fseeko(file, 0, SEEK_END)) // Go to end of file
        return -1;

    long long size = ftello(file); // Get current position byte offset
    
    if(fseeko(file, 0, SEEK_SET)) // Move back to start
        return -1;
    
    return size;
}

int main() {
    FILE* f = fopen("test_files/big.txt", "rb");
    if(!f) { perror("fopen"); return 1; }

    printf("File Size:\t%lld\n", getFileSize(f));

    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf), f);
    printf("fread returned %zu\n", n);

    fclose(f);
    return 0;
}
