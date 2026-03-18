#include "handling.h"

//  choice 1
int handle_new_download(char **filename, char *state_file,
                        int64_t *total_file_size, DownloadChunk *chunks) {

  global_url = tui_get_url();
  if (!global_url || strlen(global_url) == 0) {
    tui_cleanup();
    printf("Invalid URL.\n");
    return -1;
  }

  *filename = tokenize(global_url);
  snprintf(state_file, 256, "%s.idm_state", *filename);

  *total_file_size = get_file_size(global_url);
  if (*total_file_size <= 0) {
    tui_cleanup();
    printf("Error: Could not retrieve file size.\n");
    free(global_url);
    free(*filename);
    return -1;
  }

  int64_t chunk_size = *total_file_size / NUM_THREADS;
  for (int i = 0; i < NUM_THREADS; i++) {
    chunks[i].thread_id = i;
    chunks[i].start_byte = i * chunk_size;
    chunks[i].current_byte = chunks[i].start_byte;
    chunks[i].end_byte = (i == NUM_THREADS - 1) ? (*total_file_size - 1)
                                                : ((i + 1) * chunk_size - 1);
    chunks[i].is_finished = 0;
    snprintf(chunks[i].part_filename, sizeof(chunks[i].part_filename),
             "%s.part%d", *filename, i);
  }
  return 0;
}

// choice 2
int handle_resume_download(char **filename, char *state_file,
                           int64_t *total_file_size, int64_t *bytes_loaded,
                           DownloadChunk *chunks, int *num_chunks) {
  DIR *dir;
  struct dirent *ent;
  char *state_files[100];
  int file_count = 0;

  if ((dir = opendir(".")) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      char *dot = strrchr(ent->d_name, '.');
      if (dot && strcmp(dot, ".idm_state") == 0 && file_count < 100) {
        state_files[file_count] = strdup(ent->d_name);
        file_count++;
      }
    }
    closedir(dir);
  }

  if (file_count == 0) {
    tui_cleanup();
    printf("No paused downloads found.\n");
    return -1;
  }

  int file_choice = tui_show_resume_menu(state_files, file_count);

  if (file_choice < 1 || file_choice > file_count) {
    tui_cleanup();
    for (int i = 0; i < file_count; i++)
      free(state_files[i]);
    return -1;
  }

  strcpy(state_file, state_files[file_choice - 1]);
  for (int i = 0; i < file_count; i++)
    free(state_files[i]);

  if (!load_state(state_file, &global_url, chunks, num_chunks)) {
    tui_cleanup();
    printf("Error: Could not load state file.\n");
    return -1;
  }

  *filename = tokenize(global_url);
  *total_file_size = 0;
  *bytes_loaded = 0;

  for (int i = 0; i < *num_chunks; i++) {
    snprintf(chunks[i].part_filename, sizeof(chunks[i].part_filename),
             "%s.part%d", *filename, i);
    *total_file_size += (chunks[i].end_byte - chunks[i].start_byte + 1);
    *bytes_loaded += (chunks[i].current_byte - chunks[i].start_byte);
  }
  return 0;
}
