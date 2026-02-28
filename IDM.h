#ifndef IDM_CORE_H
#define IDM_CORE_H

#include <curl/curl.h>
#include <pthread.h>
#include <stdint.h>

// pass data to each thread
typedef struct {
    int thread_id;
    const char *url;
    int64_t start_byte;
    int64_t end_byte;
    char part_filename[256];
} DownloadChunk;

int64_t get_file_size(const char *url);
void *download_worker(void *arg);
int assemble_file(const char *final_filename, int num_chunks);

#endif // IDM_CORE_H
