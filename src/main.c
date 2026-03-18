#include "IDM.h"
#include "TUI.h"
#include "handling.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

char *global_url = NULL;

char *tokenize(const char *url_str) {
  const char *last_slash = strrchr(url_str, '/');
  if (last_slash && *(last_slash + 1) != '\0') {
    return strdup(last_slash + 1);
  }
  return strdup("downloaded_file.out");
}

// threads and UI Loop
void run_download_loop(const char *filename, int64_t total_file_size,
                       int64_t bytes_loaded_from_state, DownloadChunk *chunks,
                       int num_chunks) {
  pthread_t threads[NUM_THREADS];
  for (int i = 0; i < num_chunks; i++) {
    pthread_create(&threads[i], NULL, download_worker, &chunks[i]);
  }

  struct timeval start_time, current_time;
  gettimeofday(&start_time, NULL);

  int all_finished = 0;
  while (!all_finished && !stop_flag) {
    all_finished = 1;

    int ch = tui_check_input();
    if (ch == 's' || ch == 'q') {
      stop_flag = 1;
      break;
    }

    gettimeofday(&current_time, NULL);
    double elapsed_secs =
        (current_time.tv_sec - start_time.tv_sec) +
        (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

    int64_t total_downloaded_now = 0;
    for (int i = 0; i < num_chunks; i++) {
      total_downloaded_now += (chunks[i].current_byte - chunks[i].start_byte);
      if (!chunks[i].is_finished)
        all_finished = 0;
    }

    int64_t session_downloaded = total_downloaded_now - bytes_loaded_from_state;
    double speed_bps =
        (elapsed_secs > 0) ? (session_downloaded / elapsed_secs) : 0;
    int eta_secs = (speed_bps > 0)
                       ? ((total_file_size - total_downloaded_now) / speed_bps)
                       : 0;
    float overall_percent =
        (float)total_downloaded_now / total_file_size * 100.0f;

    tui_draw_progress(filename, total_file_size, total_downloaded_now,
                      speed_bps, eta_secs, overall_percent, chunks, num_chunks);

    usleep(100000); // 100ms refresh rate
  }

  for (int i = 0; i < num_chunks; i++) {
    pthread_join(threads[i], NULL);
  }
}

// saving state and file assembly
void finalize_download(const char *filename, const char *state_file,
                       DownloadChunk *chunks, int num_chunks) {
  if (stop_flag) {
    printf("\n\n[!] Download Paused. Saving state to %s...\n", state_file);
    save_state(state_file, global_url, chunks, num_chunks);
  } else {
    printf("\n\n[✓] Download Complete. Assembling file...\n");
    if (assemble_file(filename, num_chunks) == 0) {
      printf("[✓] Assembly successful. Cleaning up state file.\n");
      remove(state_file);
    }
  }
}

int main() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  tui_init();

  char state_file[256];
  char *filename = NULL;

  int choice = tui_show_main_menu();
  if (choice == 3) {
    tui_cleanup();
    return 0;
  }

  DownloadChunk chunks[NUM_THREADS];
  int num_chunks = NUM_THREADS;
  int64_t total_file_size = 0;
  int64_t bytes_loaded_from_state = 0;

  if (choice == 1) {
    if (handle_new_download(&filename, state_file, &total_file_size, chunks) !=
        0)
      return EXIT_FAILURE;
  } else if (choice == 2) {
    if (handle_resume_download(&filename, state_file, &total_file_size,
                               &bytes_loaded_from_state, chunks,
                               &num_chunks) != 0)
      return EXIT_FAILURE;
  }
  run_download_loop(filename, total_file_size, bytes_loaded_from_state, chunks,
                    num_chunks);
  tui_cleanup();
  finalize_download(filename, state_file, chunks, num_chunks);
  free(filename);

  curl_global_cleanup();
  return 0;
}
