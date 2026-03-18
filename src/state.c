#include "state.h"
#include "IDM.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

void save_state(const char *state_file, const char *url, DownloadChunk chunks[],
                int num_chunks) {
  FILE *fp = fopen(state_file, "w");
  if (!fp)
    return;

  fprintf(fp, "%s\n", url);
  fprintf(fp, "%d\n", num_chunks);

  for (int i = 0; i < num_chunks; i++) {
    fprintf(fp, "%d %ld %ld %ld %d\n", chunks[i].thread_id,
            chunks[i].start_byte, chunks[i].current_byte, chunks[i].end_byte,
            chunks[i].is_finished);
  }
  fclose(fp);
}

int load_state(const char *state_file, char **url_out, DownloadChunk chunks[],
               int *num_chunks) {
  FILE *fp = fopen(state_file, "r");
  if (!fp)
    return 0;

  char url_buf[2048];
  if (fscanf(fp, "%2047s", url_buf) != 1) {
    fclose(fp);
    return 0;
  }
  *url_out = strdup(url_buf);

  if (fscanf(fp, "%d", num_chunks) != 1) {
    fclose(fp);
    return 0;
  }

  for (int i = 0; i < *num_chunks; i++) {
    fscanf(fp, "%d %ld %ld %ld %d", &chunks[i].thread_id, &chunks[i].start_byte,
           &chunks[i].current_byte, &chunks[i].end_byte,
           &chunks[i].is_finished);
  }
  fclose(fp);
  return 1;
}
