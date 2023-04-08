#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>

#include "args.h"

static size_t writeOutputFileData(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

static off_t urlGetContentLength(const char *url, const char *dlFolder)
{
    char filePath[512];
    FILE *fp;
    CURL *ch;
    char errBuf[CURL_ERROR_SIZE];
    off_t contentLength = 0;

    snprintf(filePath, sizeof (filePath), "%s/%s", dlFolder, "headerData.txt");

    if ((fp = fopen(filePath, "wb")) == NULL) {
        fprintf(stderr, "ERROR: can't open HEADERDATA file \"%s\" (%s)\n", filePath, strerror(errno));
        return -1;
    }

    // Init the curl session
    ch = curl_easy_init();

    // Set URL to get here
    curl_easy_setopt(ch, CURLOPT_URL, url);

    // Set to 1L to enable full debug
    curl_easy_setopt(ch, CURLOPT_VERBOSE, 0L);

    // Set to 0L to enable download progress meter
    curl_easy_setopt(ch, CURLOPT_NOPROGRESS, 1L);

    // Send all data to this function
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, writeOutputFileData);

    // Write the header data to this file handle
    curl_easy_setopt(ch, CURLOPT_HEADERDATA, fp);

    // Don't download the page data!
    curl_easy_setopt(ch, CURLOPT_NOBODY, 1L);

    // Buffer where to store error message
    curl_easy_setopt(ch, CURLOPT_ERRORBUFFER, errBuf);

    // Go fetch!
    if (curl_easy_perform(ch) != CURLE_OK) {
        fprintf(stderr, "ERROR: file download failed (%s)\n", errBuf);
    }

    // Cleanup curl stuff
    curl_easy_cleanup(ch);

    fclose(fp);

    // Get the 'content-length' value
    {
        char lineBuf[256];

        if ((fp = fopen(filePath, "r")) == NULL) {
            fprintf(stderr, "ERROR: can't re-open HEADERDATA file \"%s\" (%s)\n", filePath, strerror(errno));
            return -1;
        }

        while (fgets(lineBuf, sizeof (lineBuf), fp) != NULL) {
            //printf("%s", lineBuf);
            if (sscanf(lineBuf, "content-length: %" PRId64 "", &contentLength) == 1)
                break;
        }

        fclose(fp);
    }

    unlink(filePath);

    return contentLength;
}

int urlDownload(const char *url, const char *outFile, const CmdArgs *pArgs)
{
    char filePath[512];
    struct stat statBuf;

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
    snprintf(filePath, sizeof (filePath), "%s/%s", pArgs->dlFolder, outFile);

    // See if the file already exists in the
    // download folder...
    if ((stat(filePath, &statBuf) == 0) && (statBuf.st_mode & S_IFREG)) {
        off_t contentLength = urlGetContentLength(url, pArgs->dlFolder);
        if (contentLength == statBuf.st_size) {
            printf("INFO: Skipping file %s because it already exists in the specified download folder.\n", outFile);
            return 0;
        } else {
            printf("INFO: File %s already exists in the specified download folder, but with a different size: url=%" PRId64 " file=%" PRId64 "\n",
                    outFile, contentLength, statBuf.st_size);
        }
    }

    if (!pArgs->dryRun) {
        FILE *fp;
        CURL *ch;
        char errBuf[CURL_ERROR_SIZE];

        // Open the output file
        if ((fp = fopen(filePath, "wb")) == NULL) {
            fprintf(stderr, "ERROR: can't open output file \"%s\" (%s)\n", filePath, strerror(errno));
            return -1;
        }

        printf("Downloading: %s ....\n", url);

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
        }

        // Cleanup curl stuff
        curl_easy_cleanup(ch);

        // Close the output file
        fclose(fp);
    } else {
        printf("Would download: %s ....\n", url);
    }

    return 0;
}
