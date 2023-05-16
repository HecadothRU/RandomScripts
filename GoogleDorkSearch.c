#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pcre2.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *getPageSource(const char *url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.0.3705;)");

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk.memory;
}

char **getMatches(const char *toSearch, const char *regexPattern, int *count) {
    pcre2_code *re;
    PCRE2_SPTR pattern = (PCRE2_SPTR)regexPattern;
    PCRE2_SPTR subject = (PCRE2_SPTR)toSearch;
    int errornumber;
    size_t erroroffset;
    pcre2_match_data *match_data;
    int rc;

    re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, NULL);
    if (re == NULL) {
        printf("PCRE2 compilation failed at offset %zu: %d\n", erroroffset, errornumber);
        return NULL;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    rc = pcre2_match(re, subject, strlen(toSearch), 0, 0, match_data, NULL);

    if (rc < 0) {
        printf("No match found\n");
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return NULL;
    }

    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
    *count = rc;
    char **matches = malloc(rc * sizeof(char *));

    for (int i = 0; i < rc; i++) {
        PCRE2_SIZE match_length = ovector[2 * i        +1] - ovector[2 * i];
        matches[i] = malloc((match_length + 1) * sizeof(char));
        memcpy(matches[i], &subject[ovector[2 * i]], match_length);
        matches[i][match_length] = '\0';
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    return matches;
}

int isUri(const char *source) {
    CURLU *h;
    CURLUcode uc;

    h = curl_url();
    uc = curl_url_set(h, CURLUPART_URL, source, 0);
    curl_url_cleanup(h);

    if (!uc) {
        return 1;
    }

    return 0;
}

char **searchGoogle(const char *query, int *resultCount) {
    char url[256];
    snprintf(url, sizeof(url), "http://www.google.com/search?num=100&q=%s", curl_easy_escape(NULL, query, 0));
    char *pageSource = getPageSource(url);

    int matchCount;
    char **matches = getMatches(pageSource, "/url\\?q=(.*?)&/", &matchCount);

    char **result = malloc(matchCount * sizeof(char *));
    *resultCount = 0;

    for (int i = 0; i < matchCount; i++) {
        if (!strstr(matches[i], "googleusercontent") && !strstr(matches[i], "/settings/ads")) {
            result[*resultCount] = malloc((strlen(matches[i]) + 256) * sizeof(char));
            sprintf(result[*resultCount], "<a href=\"%s\">Possible Connection: %s</a><br>", matches[i], matches[i]);
            (*resultCount)++;
        }
        free(matches[i]);
    }
    free(matches);
    free(pageSource);

    return result;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid arguments. Usage: ./program <query>\n");
        return 1;
    }

    int resultCount;
    char **results = searchGoogle(argv[1], &resultCount);

    for (int i = 0; i < resultCount; i++) {
        printf("%s\n", results[i]);
        free(results[i]);
    }
    free(results);

    return 0;
}
      
