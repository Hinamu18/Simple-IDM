#ifndef STATE_H
#define STATE_H

#include "IDM.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

void save_state(const char *state_file, const char *url, DownloadChunk chunks[],
                int num_chunks);

int load_state(const char *state_file, char **url_out, DownloadChunk chunks[],
               int *num_chunks);

#endif // STATE_H
