#ifndef TUI_H
#define TUI_H

#include "IDM.h"
#include <stdint.h>

void tui_init(void);
void tui_cleanup(void);

int tui_check_input(void);

int tui_show_main_menu(void);
char *tui_get_url(void);
int tui_show_resume_menu(char *state_files[], int file_count);

void tui_draw_progress(const char *filename, int64_t total_file_size,
                       int64_t total_downloaded_now, double speed_bps,
                       int eta_secs, float overall_percent,
                       DownloadChunk *chunks, int num_chunks);

#endif // TUI_H
