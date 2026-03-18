#include "IDM.h"
#include <stdlib.h>

volatile int stop_flag = 0;

// write_data also updates current_byte
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  DownloadChunk *chunk = (DownloadChunk *)stream;
  size_t written = fwrite(ptr, size, nmemb, chunk->fp);
  chunk->current_byte += written;
  return written;
}

static int progress_callback(void *clientp, curl_off_t dltotal,
                             curl_off_t dlnow, curl_off_t ultotal,
                             curl_off_t ulnow) {
  if (stop_flag) {
    return 1; // abort
  }
  return 0; // continue
}

#include <curl/curl.h>
#include <stdio.h>

// this callback is triggered if the body data starts arriving
// returning 0 tells libcurl to abort the transfer
size_t abort_download_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
  return 0;
}

int64_t get_file_size(const char *url) {
  CURL *curl = curl_easy_init();
  curl_off_t file_size = -1;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // write callback aborts the transfer
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, abort_download_cb);

    CURLcode res = curl_easy_perform(curl);

    // callback aborted it
    // so now i can debug the request and try to get the length
    // TODO
    if (res == CURLE_WRITE_ERROR || res == CURLE_OK) {
      long response_code = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);
    }

    curl_easy_cleanup(curl);
  }

  return (int64_t)file_size;
}

void *download_worker(void *arg) {
  DownloadChunk *chunk = (DownloadChunk *)arg;

  // if already finished from a previous run, exit
  if (chunk->current_byte > chunk->end_byte) {
    chunk->is_finished = true;
    return NULL;
  }

  CURL *curl = curl_easy_init();
  if (curl) {
    // NOTE: open in Append Binary ("ab") so we don't overwrite previous
    // progress
    chunk->fp = fopen(chunk->part_filename, "ab");
    if (!chunk->fp) {
      curl_easy_cleanup(curl);
      return NULL;
    }

    char range[128];
    snprintf(range, sizeof(range), "%ld-%ld", (long)chunk->current_byte,
             (long)chunk->end_byte);

    curl_easy_setopt(curl, CURLOPT_URL, global_url);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      chunk->is_finished = true;
    }

    fclose(chunk->fp);
    curl_easy_cleanup(curl);
  }
  return NULL;
}

int assemble_file(const char *final_filename, int num_chunks) {
  FILE *fp_out = fopen(final_filename, "wb");
  if (!fp_out)
    return -1;

  char buffer[4092];
  size_t bytes_read;

  for (int i = 0; i < num_chunks; i++) {
    char part_filename[256];
    snprintf(part_filename, sizeof(part_filename), "%s.part%d", final_filename,
             i);

    FILE *fp_in = fopen(part_filename, "rb");
    if (!fp_in) {
      fclose(fp_out);
      return -1;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp_in)) > 0) {
      fwrite(buffer, 1, bytes_read, fp_out);
    }

    fclose(fp_in);
    remove(part_filename);
  }

  fclose(fp_out);
  return 0;
}
