#ifndef HANDLING_H
#define HANDLING_H
#include "IDM.h"
#include "TUI.h"
#include "state.h"
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int handle_new_download(char **filename, char *state_file,
                        int64_t *total_file_size, DownloadChunk *chunks);
int handle_resume_download(char **filename, char *state_file,
                           int64_t *total_file_size, int64_t *bytes_loaded,
                           DownloadChunk *chunks, int *num_chunks);

#endif // HANDLING_H
