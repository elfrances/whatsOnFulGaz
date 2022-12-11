#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include "args.h"

static size_t writeOutputFileData(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

int urlDownload(const char *url, const char *outFile, const CmdArgs *pArgs)
{
    char filePath[512];
    FILE *fp;
    CURL *ch;
    char errBuf[CURL_ERROR_SIZE];

    printf("Downloading: %s ....\n", url);

    // If no outfile has been specified, use the URL's
    // basename as the output file name.
    if (outFile == NULL) {
        if ((outFile = strrchr(url, '/')) == NULL) {
            // Hu?
            fprintf(stderr, "ERROR: can't figure out output file name\n");
            return -1;
        }
        outFile++;  // skip the final '/'
    }

    // Figure out the path to the output file
    if (pArgs->dlFolder != NULL) {
        snprintf(filePath, sizeof (filePath), "%s/%s", pArgs->dlFolder, outFile);
    } else {
        snprintf(filePath, sizeof (filePath), "%s", outFile);
    }

    // Open the output file
    if ((fp = fopen(filePath, "wb")) == NULL) {
        fprintf(stderr, "ERROR: can't open output file \"%s\" (%s)\n", filePath, strerror(errno));
        return -1;
    }

    // Init the curl session
    ch = curl_easy_init();

    // Set URL to get here
    curl_easy_setopt(ch, CURLOPT_URL, url);

    // Set to 1L to enable full debug
    curl_easy_setopt(ch, CURLOPT_VERBOSE, 0L);

    // Set to 0L to enable download progress meter
    curl_easy_setopt(ch, CURLOPT_NOPROGRESS, (pArgs->dlProg) ? 0L : 1L);

    // Send all data to this function
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, writeOutputFileData);

    // Write the page body to this file handle
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, fp);

    // Buffer where to store error message
    curl_easy_setopt(ch, CURLOPT_ERRORBUFFER, errBuf);

    // Go fetch!
    if (curl_easy_perform(ch) != CURLE_OK) {
        fprintf(stderr, "ERROR: file download failed (%s)\n", errBuf);
        return -1;
    }

    // Cleanup curl stuff
    curl_easy_cleanup(ch);

    // Close the output file
    fclose(fp);

    return 0;
}
