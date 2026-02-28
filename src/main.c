#include "IDM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

char* tokenize(char *str){
	
    char *res;
    int i = 0, index = -1;
    int len = strlen(str);

    while (str[i] != '\0') {
        if (str[i] == '/') {
            index = i+1;
        }
        i++;
    }

    res = malloc(index + 1);
    for (int j = 0; j < index; j++) {
        res[j] = str[j+index];
    }

    res[index] = '\0';

    printf("%s", res);
    return res;
}

int main(int argc, char *argv[]) {

	// 1. Call get_file_size()
	// 2. Calculate chunk boundaries
	// 3. Spawn pthreads using download_worker()
	// 4. Join pthreads
	// 5. Call assemble_file()

	if (argc > 2) {
		printf("Usage: %s <url>",argv[0]);
		return EXIT_FAILURE;
	}

	CURL *curl = curl_easy_init();
	
	char *output = tokenize(argv[1]);
	FILE *fp = fopen(output, "wb");

	if (!curl) {
		printf("curl init error");
		return EXIT_FAILURE;
	}


	curl_easy_setopt(curl,CURLOPT_URL,argv[1]);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	fclose(fp);
	curl_global_cleanup();
	free(output);


	return 0;
}
