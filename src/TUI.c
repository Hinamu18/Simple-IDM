#include "TUI.h"
#include <ncurses.h>
#include <string.h>

#define COLOR_BORDER 1
#define COLOR_TEXT 2
#define COLOR_HIGHLIGHT 3
#define COLOR_PROGRESS_DONE 4
#define COLOR_PROGRESS_PENDING 5

void tui_init(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);            // hide cursor
  nodelay(stdscr, FALSE); // default to blocking for menus

  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(COLOR_BORDER, COLOR_CYAN, -1);
    init_pair(COLOR_TEXT, COLOR_WHITE, -1);
    init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
    init_pair(COLOR_PROGRESS_DONE, COLOR_GREEN, -1);
    init_pair(COLOR_PROGRESS_PENDING, COLOR_WHITE, -1);
  }
}

void tui_cleanup(void) {
  curs_set(1);
  endwin();
}

int tui_check_input(void) {
  nodelay(stdscr, TRUE); // switch to non-blocking

  int ch = getch();

  nodelay(stdscr, FALSE); // switch back
  if (ch != ERR) {
    return ch;
  }
  return 0;
}

static void format_size(double bytes, char *output) {
  const char *units[] = {"B", "KB", "MB", "GB"};

  int i = 0;
  while (bytes >= 1024.0 && i < 3) {
    bytes /= 1024.0;
    i++;
  }

  sprintf(output, "%.2f %s", bytes, units[i]);
}

static WINDOW *create_centered_win(int height, int width) {
  int starty = (LINES - height) / 2;
  int startx = (COLS - width) / 2;
  WINDOW *win = newwin(height, width, starty, startx);

  wattron(win, COLOR_PAIR(COLOR_BORDER));
  box(win, 0, 0);
  wattroff(win, COLOR_PAIR(COLOR_BORDER));

  return win;
}

int tui_show_main_menu(void) {
  const char *choices[] = {"Start a New Download",
                           "Resume an Existing Download", "Quit"};
  int choices_index = 3;
  int highlight = 0;
  int choice = -1;

  int height = 10;
  int width = 45;
  WINDOW *menu_win = create_centered_win(height, width);
  keypad(menu_win, TRUE);

  mvwprintw(menu_win, 1, (width - 25) / 2, "TERMINAL DOWNLOAD MANAGER");
  mvwhline(menu_win, 2, 1, ACS_HLINE, width - 2);

  while (1) {

    for (int i = 0; i < choices_index; i++) {
      if (i == highlight)
        wattron(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT));
      mvwprintw(menu_win, 4 + i, (width - strlen(choices[i])) / 2, "%s",
                choices[i]);
      if (i == highlight)
        wattroff(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT));
    }
    wrefresh(menu_win);

    int c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
      if (highlight > 0)
        highlight--;
      break;
    case KEY_DOWN:
      if (highlight < choices_index - 1)
        highlight++;
      break;
    case 10: // enter
      choice = highlight + 1;
      break;
    }
    if (choice != -1)
      break;
  }
  delwin(menu_win);
  clear();
  refresh();
  return choice;
}

char *tui_get_url(void) {
  int height = 5;
  int width = 60;
  WINDOW *input_win = create_centered_win(height, width);

  mvwprintw(input_win, 1, 2, "Enter URL:");
  wrefresh(input_win);

  char input_buf[4092] = {0};

  curs_set(1);
  echo();
  mvwgetnstr(input_win, 2, 2, input_buf, sizeof(input_buf) - 1);
  noecho();
  curs_set(0);

  delwin(input_win);
  clear();
  refresh();

  if (strlen(input_buf) > 0) {
    return strdup(input_buf);
  }
  return NULL;
}

int tui_show_resume_menu(char *state_files[], int file_count) {
  int highlight = 0;
  int choice = -1;

  int height = file_count + 6;
  if (height > LINES - 2)
    height = LINES - 2;
  int width = 50;
  WINDOW *menu_win = create_centered_win(height, width);
  keypad(menu_win, TRUE);

  mvwprintw(menu_win, 1, (width - 15) / 2, "RESUME DOWNLOAD");
  mvwhline(menu_win, 2, 1, ACS_HLINE, width - 2);

  while (1) {
    for (int i = 0; i < file_count; i++) {
      char display_name[256];
      strncpy(display_name, state_files[i], sizeof(display_name) - 1);
      char *ext = strstr(display_name, ".idm_state");
      if (ext)
        *ext = '\0';

      if (i == highlight)
        wattron(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT));
      mvwprintw(menu_win, 4 + i, 2, "[%d] %-40.40s", i + 1, display_name);
      if (i == highlight)
        wattroff(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT));
    }
    wrefresh(menu_win);

    int c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
      if (highlight > 0)
        highlight--;
      break;
    case KEY_DOWN:
      if (highlight < file_count - 1)
        highlight++;
      break;
    case 10:
      choice = highlight + 1;
      break;
    case 27:
    case 'q':
      choice = 0;
      break;
    }
    if (choice != -1)
      break;
  }
  delwin(menu_win);
  clear();
  refresh();
  return choice;
}

void tui_draw_progress(const char *filename, int64_t total_file_size,
                       int64_t total_downloaded_now, double speed_bps,
                       int eta_secs, float overall_percent,
                       DownloadChunk *chunks, int num_chunks) {
  char speed_str[32], total_str[32], dl_str[32];
  format_size(speed_bps, speed_str);
  format_size(total_file_size, total_str);
  format_size(total_downloaded_now, dl_str);

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  erase(); // Wipes the screen clean every frame to prevent
           // visual artifacts

  attron(COLOR_PAIR(COLOR_BORDER));
  box(stdscr, 0, 0);
  attroff(COLOR_PAIR(COLOR_BORDER));

  mvprintw(1, 2, "File: %s", filename);
  mvprintw(2, 2, "Size: %s / %s (%.1f%%)", dl_str, total_str, overall_percent);
  mvprintw(3, 2, "Speed: %s/s  |  ETA: %02d:%02d", speed_str, eta_secs / 60,
           eta_secs % 60);

  mvhline(4, 1, ACS_HLINE, max_x - 2);

  int start_y = 6;
  for (int i = 0; i < num_chunks; i++) {
    int64_t total = chunks[i].end_byte - chunks[i].start_byte + 1;
    int64_t downloaded = chunks[i].current_byte - chunks[i].start_byte;
    if (downloaded > total)
      downloaded = total;

    float percent = (total > 0) ? (float)downloaded / total * 100.0f : 0.0f;

    int bar_width = max_x - 30;
    if (bar_width < 10)
      bar_width = 10;
    int fill_len = (int)((percent / 100.0f) * bar_width);

    mvprintw(start_y + i * 2, 2, "Seg %d: %5.1f%% [", i + 1, percent);

    attron(COLOR_PAIR(COLOR_PROGRESS_DONE));
    for (int b = 0; b < fill_len; b++)
      printw("#");
    attroff(COLOR_PAIR(COLOR_PROGRESS_DONE));

    attron(COLOR_PAIR(COLOR_PROGRESS_PENDING));
    for (int b = fill_len; b < bar_width; b++)
      printw("-");
    attroff(COLOR_PAIR(COLOR_PROGRESS_PENDING));

    printw("]");
  }

  mvhline(max_y - 3, 1, ACS_HLINE, max_x - 2);
  mvprintw(max_y - 2, 2, "Press [s] to Pause & Save State | Press [q] to Quit");

  refresh(); // Apply the drawing
}
