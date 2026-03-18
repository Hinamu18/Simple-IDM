#ifndef IDM_CORE_H
#define IDM_CORE_H

#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_THREADS 4

char *tokenize(const char *url_str);

extern char *global_url;
extern volatile int stop_flag; // for threads to stop

typedef struct {
  int thread_id;
  int64_t start_byte;
  int64_t current_byte; // tracks progress for resuming
  int64_t end_byte;
  char part_filename[256];
  FILE *fp;
  bool is_finished; // 1 if this chunk is 100% done
} DownloadChunk;

int64_t get_file_size(const char *url);
void *download_worker(void *arg);
int assemble_file(const char *final_filename, int num_chunks);

#endif // IDM_CORE_H
