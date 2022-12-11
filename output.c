#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "args.h"
#include "routedb.h"

// A few routes include a comma in their description which
// screws up the CSV output format...
static char *fmtTitle(const char *title)
{
    char *p;
    static char fmtBuf[128];

    snprintf(fmtBuf, sizeof (fmtBuf), "%s", title);
    if ((p = strchr(fmtBuf, ',')) != NULL) {
        *p = '\0';
    }

    return fmtBuf;
}

// Extract the country from the location string, which looks
// like this:
//   "Barossa Valley, South Australia, Australia"
//
static char *fmtCountry(const char *location)
{
    char *p;
    static char fmtBuf[128];

    if ((p = strrchr(location, ',')) != NULL) {
        for (p = p+1; *p != '\0'; p++) {
            int c = *p;
            if (!isspace(c))
                break;
        }
        snprintf(fmtBuf, sizeof (fmtBuf), "%s", p);
    } else if (location[0] != '\0') {
        snprintf(fmtBuf, sizeof (fmtBuf), "%s", location);
    } else {
        snprintf(fmtBuf, sizeof (fmtBuf), "???");
    }

    return fmtBuf;
}

// Format time as HH:MM:SS
static char *fmtTime(int time)
{
    static char fmtBuf[128];
    int hr, min, sec;

    hr = time / 3600;
    min = (time - (hr * 3600)) / 60;
    sec = (time - (hr * 3600) - (min * 60));
    snprintf(fmtBuf, sizeof (fmtBuf), "%02d:%02d:%02d", hr, min, sec);

    return fmtBuf;
}

static const char *cellName[] = {
        "Name",
        "Country",
        "Contributor",
        "Distance",
        "Elevation Gain",
        "Duration",
        "Toughness Score",
        "720p Video",
        "1080p Video",
        "4K Video",
        "SHIZ",
        NULL
};

void printCsvOutput(const RouteDB *pDb)
{
    RouteInfo *pRoute;

    for (int n = 0; cellName[n] != NULL; n++) {
        printf("%s,", cellName[n]);
    }
    printf("\n");

    TAILQ_FOREACH(pRoute, &pDb->routeList, tqEntry) {
        printf("%s,", fmtTitle(pRoute->title));
        printf("%s,", fmtCountry(pRoute->location));
        printf("%s,", pRoute->contributor);
        printf("%s,", pRoute->distance);
        printf("%s,", pRoute->elevation);
        printf("%s,", fmtTime(pRoute->time));
        printf("%s,", pRoute->toughness);
        printf("%s%s,", pDb->mp4UrlPfx, pRoute->vim720);
        printf("%s%s,", pDb->mp4UrlPfx, pRoute->vim1080);
        printf("%s%s,", pDb->mp4UrlPfx, pRoute->vimMaster);
        printf("%s%s,", pDb->shizUrlPfx, pRoute->shiz);
        printf("\n");
    }
}

static void printStringCellValue(const char *string, int boldFace)
{
    printf("                <td width=\"10%%\" style=\"border-top: 1px solid #000000; border-bottom: 1px solid #000000; border-left: 1px solid #000000; border-right: none; padding-top: 0.04in; padding-bottom: 0.04in; padding-left: 0.04in; padding-right: 0in\">\n");
    if (boldFace) {
        printf("                    <p><font face=\"Tahoma, sans-serif\"><b>%s</b></font></p>\n", string);
    } else {
        printf("                    <p><font face=\"Tahoma, sans-serif\">%s</font></p>\n", string);
    }
    printf("                </td>\n");
}

static void printHyperlinkCellValue(const char *string)
{
    printf("                <td width=\"10%%\" style=\"border-top: 1px solid #000000; border-bottom: 1px solid #000000; border-left: 1px solid #000000; border-right: none; padding-top: 0.04in; padding-bottom: 0.04in; padding-left: 0.04in; padding-right: 0in\">\n");
    printf("                    <p><a href=\"%s\"><font face=\"Tahoma, sans-serif\">link</font></a></p>\n", string);
    printf("                </td>\n");
}

void printHttpOutput(const RouteDB *pDb)
{
    RouteInfo *pRoute;

    printf("<html>\n");
    printf("    <head>\n");
    printf("        <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>\n");
    printf("        <title>FulGaz Route Library</title>\n");
    printf("    </head>\n");
    printf("    <body lang=\"en-US\" link=\"#000080\" vlink=\"#800000\" dir=\"ltr\">\n");
    printf("        <table width=\"100%%\" cellpadding=\"4\" cellspacing=\"0\">\n");
    for (int n = 0; cellName[n] != NULL; n++) {
        printf("            <col width=\"26*\"/>\n");
    }
    printf("            <tr valign=\"top\">\n");
    for (int n = 0; cellName[n] != NULL; n++) {
        printStringCellValue(cellName[n], 1);
    }
    printf("            </tr>\n");
    TAILQ_FOREACH(pRoute, &pDb->routeList, tqEntry) {
        char link[256];
        printf("            <tr valign=\"top\">\n");
        printStringCellValue(fmtTitle(pRoute->title), 0);
        printStringCellValue(fmtCountry(pRoute->location), 0);
        printStringCellValue(pRoute->contributor, 0);
        printStringCellValue(pRoute->distance, 0);
        printStringCellValue(pRoute->elevation, 0);
        printStringCellValue(fmtTime(pRoute->time), 0);
        printStringCellValue(pRoute->toughness, 0);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vim720);
        printHyperlinkCellValue(link);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vim1080);
        printHyperlinkCellValue(link);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vimMaster);
        printHyperlinkCellValue(link);
        snprintf(link, sizeof (link), "%s%s", pDb->shizUrlPfx, pRoute->shiz);
        printHyperlinkCellValue(link);
        printf("            </tr>\n");
    }
    printf("        </table>\n");
    printf("    </body>\n");
    printf("</html>\n");
}

void printTextOutput(const RouteDB *pDb)
{
    RouteInfo *pRoute;

    TAILQ_FOREACH(pRoute, &pDb->routeList, tqEntry) {
        char link[256];
        printf("{\n");
        printf("    Name:            %s\n", fmtTitle(pRoute->title));
        printf("    Country:         %s\n", fmtCountry(pRoute->location));
        printf("    Contributor:     %s\n", pRoute->contributor);
        printf("    Distance:        %s\n", pRoute->distance);
        printf("    Elevation Gain:  %s\n", pRoute->elevation);
        printf("    Duration:        %s\n", fmtTime(pRoute->time));
        printf("    Toughness Score: %s\n", pRoute->toughness);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vim720);
        printf("    720p Video:      %s\n", link);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vim1080);
        printf("    1080p Video:     %s\n", link);
        snprintf(link, sizeof (link), "%s%s", pDb->mp4UrlPfx, pRoute->vimMaster);
        printf("    4K Video:        %s\n", link);
        snprintf(link, sizeof (link), "%s%s", pDb->shizUrlPfx, pRoute->shiz);
        printf("    SHIZ:            %s\n", link);
        printf("}\n");
    }
}

