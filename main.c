#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

/*
 * In FulGaz a route record is a JSON object with the
 * following structure:
 *
 * {
 * 		"_id":"f1b5ca99119ffe68fbe6f0fa",
 * 		"appId":"55f780c682d4002ba4cc479077db57a400cbe240",
 * 		"vim1080":{
 * 			"file":"1080P/Col_de_Vars.mp4",
 * 			"sha":"25e786e7a89ebbe330ebabbc6e8ba499957a4a013781332363adbba55e2f2044"
 * 		},
 * 		"vim720":{
 * 			"file":"720P/Col_de_Vars.mp4",
 * 			"sha":"68faac82299ec6097b4a6d22c0cff3efecbcc9d7c4397250b0f8e4d062a1ba9d"
 * 		},
 * 		"meta":{
 * 			"country":"all",
 * 			"dur":"1:08:21",
 * 			"dis":"15.26",
 * 			"des":"A relatively easy 5% start, although on very hot south-facing slopes, with the toughest sections at more than 10% in the second half.  Filmed on the 2017 Étape du Tour.  Used on stage 18 of the 2017 Tour de France, the first rider over the summit at 2100m was Alexey Lutsenko (Astana) from Kazakhstan",
 * 			"cat":["Hilly"],
 * 			"ele":"682",
 * 			"tou":"473",
 * 			"loc":"Hautes-Alpes/Alpes-de-Haute-Provence, France",
 * 			"con":"Franck Villano",
 * 			"ter":""
 * 		},
 * 		"compType":"single",
 * 		"vimMaster":{
 * 			"file":""
 * 		},
 * 		"views":173,
 * 		"hls":"https://fulgazhls.cachefly.net/file/fulgaz-videos/,1080P,720P,/Col_de_Vars.mp4.cf/master.m3u8",
 * 		"avatarMode":0,
 * 		"forceDownload":false,
 * 		"product":"fulgaz",
 * 		"workoutURL":"",
 * 		"a":{
 * 			"image":["Col_de_Vars.jpg"],
 * 			"file":["Col_de_Vars-seg.shiz"]
 * 		},
 * 		"u":1517239331176,
 * 		"loc":{
 * 			"lat":44.509897,
 * 			"lon":6.746794
 * 		},
 * 		"t":"Étape du Tour 2017 - Col de Vars from Saint-Paul-sur-Ubaye "
 * }
 *
 * File structure:
 *
 * {
 * 		"result":"success",
 *  	"prefix":"https://fulgaz.cachefly.net/file/fulgaz-videos/",
 *  	"data":[<rec1>,<rec2>,...<recN>]
 * }
 *
 */

typedef enum OutFmt {
    undef = 0,
    csv = 1,
    html = 2,
} OutFmt;

typedef enum VidRes {
    any = 0,
    res720p = 1,
    res1080p = 2,
    res4K = 3,
} VidRes;

typedef struct CmdArgs {
    const char *contributor;
    const char *country;
    OutFmt outFmt;
    VidRes vidRes;
    int getVideo;
} CmdArgs;

typedef struct JsonObject {
    char *start;    // points to the left curly brace where the record starts
    char *end;      // points to the right curly brace where the record ends
} JsonObject;

typedef struct RouteInfo {
	TAILQ_ENTRY(RouteInfo) tqEntry;

	char *categories;	//
    char *contributor;  // Contributor
    char *distance;     // Distance (in km)
    char *duration;     // Duration of the video (HH:MM:SS)
    char *elevation;    // Elevation gain (in meters)
    char *id;           // Route ID
    char *location;     // Location
    char *shiz;         // SHIZ control file
    char *title;        // Title
    char *toughness;    // Toughness score
    char *vimMaster;    // 4K video file
    char *vim1080;      // 1080p video file
    char *vim720;       // 720p video file

    int time;           // duration (in seconds)
} RouteInfo;

typedef struct RouteDB {
	// URL prefix for fetching the MP4 file of a route
	char *mp4UrlPfx;

	// List of routes
	TAILQ_HEAD(RouteList, RouteInfo) routeList;

	// Number of routes in the list
	int numRoutes;
} RouteDB;

// URL prefix for fetching the SHIZ file of a route
const char *shizUrlPfx = "https://video.fulgaz.com/";

static char *stristr(const char *s1, const char *s2 )
{
    const char *p1 = s1 ;
    const char *p2 = s2 ;
    const char *r = *p2 == 0 ? s1 : 0 ;

    while ((*p1 != 0) && (*p2 != 0)) {
        if (tolower(*p1) == tolower(*p2)) {
            if (r == 0) {
                r = p1;
            }
            p2++;
        } else {
            p2 = s2;
            if (r != 0) {
                p1 = r + 1;
            }

            if (tolower(*p1) == tolower(*p2)) {
                r = p1;
                p2++;
            } else {
                r = 0;
            }
        }

        p1++;
    }

    return (*p2 == 0) ? (char *) r : NULL;
}

// Create a null-terminated string with the characters
// between 'start' and 'end' inclusive.
static char *stringify(const char *start, const char *end)
{
    char *str;
    size_t len = end - start + 1;

    if ((str = malloc(len+1)) != NULL) {
        memcpy(str, start, len);
        str[len] = '\0';
    }

    return str;
}

#if 0
static void dumpText(const char *data, size_t dataLen)
{
	const char *pEnd = data + dataLen;
    for (const char *p = data; p <= pEnd; p++) {
        fputc(*p, stdout);
    }
    fputc('\n', stdout);
    fflush(stdout);
}

static void jsonDumpObject(const JsonObject *pObj)
{
	dumpText(pObj->start, (pObj->end - pObj->start));
}
#endif

// Locate the specified tag within the given JSON object and
// return a pointer to its value: e.g.
//
//   { ..., <tag> : <value>, ... }
//
static const char *jsonFindTag(const JsonObject *pObj, const char *tag)
{
    char label[256];
    size_t len;

    snprintf(label, sizeof (label), "\"%s\"", tag);
    len = strlen(label);
    for (const char *p = (pObj->start + 1); p < pObj->end; p++) {
    	if (memcmp(p, label, len) == 0) {
    		for (p += len; p < pObj->end; p++) {
    			int c = *p;
    			if (isspace(c) || (c == ':'))
    				continue;
    			return p;
    		}
    	}
    }

    return NULL;
}

// Format is: "<tag>":"<val>" where the value is a string
static int jsonGetStringValue(const JsonObject *pObj, const char *tag, char **pVal)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
		const char *openQuotes = strchr(val, '"');
		if (openQuotes != NULL) {
			const char *endQuotes = strchr((openQuotes+1), '"');
			if (endQuotes != NULL) {
				char *str = stringify((openQuotes+1), (endQuotes - 1));
				if (str != NULL) {
					*pVal = str;
					return 0;
				}
			}
		}
    }

    return -1;
}

// Format is: "<tag>":[<ent0>,<ent1>,...,<entN>]
static int jsonGetArrayValue(const JsonObject *pObj, const char *tag, char **pVal)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
    	// Locate the left square bracket
		const char *leftBracket = strchr(val, '[');
		if (leftBracket != NULL) {
	        // Locate the matching right square bracket
			int level = 0;
	        for (const char *p = leftBracket; p != pObj->end; p++) {
	            if (*p == '[') {
	                level++;
	            } else if (*p == ']') {
	                level--;
	            }
	            if (level == 0) {
	            	const char *rightBracket = p;
					char *str = stringify(leftBracket, rightBracket);
					if (str != NULL) {
						*pVal = str;
						return 0;
					}
	            }
	        }
		}
    }

    return -1;
}

// A JSON object consists of text enclosed within matching
// curly braces: e.g.
//
//    {"user':"John Doe","age":"35","gender":"male"}
//
static int jsonFindObject(const char *data, size_t dataLen, JsonObject *pObj)
{
    // Locate the left curly brace
    if ((pObj->start = strchr(data, '{')) != NULL) {
        int level = 0;
        const char *pEnd = data + dataLen;

        // Locate the matching right curly brace which
        // terminates the JSON object.
        for (char *p = pObj->start; p != pEnd; p++) {
            if (*p == '{') {
                level++;
            } else if (*p == '}') {
                level--;
            }
            if (level == 0) {
            	pObj->end = p;
                return 0;
            }
        }
    }

    return -1;
}

// Search the current object for an embedded object with
// the given tag: e.g.
//
//   { ..., "info":{"make":"Honda","model":"Civic","trim":"EX"}, ...}
//
static int jsonFindObjByTag(const JsonObject *pObj, const char *tag, JsonObject *pEmbObj)
{
    const char *lbl;

    if ((lbl = jsonFindTag(pObj, tag)) != NULL) {
    	size_t dataLen = (pObj->end - lbl);
    	return jsonFindObject(lbl, dataLen, pEmbObj);
    }

    return -1;
}

static int applyMatchFilters(const RouteInfo *pInfo, const CmdArgs *pArgs)
{
    if ((pArgs->contributor != NULL) && (stristr(pInfo->contributor, pArgs->contributor) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->country != NULL) && (stristr(pInfo->location, pArgs->country) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->vidRes == res720p) && (pInfo->vim720 == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->vidRes == res1080p) && (pInfo->vim1080 == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->vidRes == res4K) && (pInfo->vimMaster == NULL)) {
        // Ignore this ride...
        return -1;
    }

    return 0;
}

static int procRouteObj(RouteDB *pDb, const JsonObject *pRoute, const CmdArgs *pArgs)
{
	RouteInfo info = {0};

	//jsonDumpObject(pRoute);

	if (jsonGetStringValue(pRoute, "_id", &info.id) != 0) {
		fprintf(stderr, "ERROR: failed to get \"_id\" value!\n");
		return -1;
	}

	if (jsonGetStringValue(pRoute, "t", &info.title) != 0) {
		fprintf(stderr, "ERROR: failed to get \"t\" value!\n");
		return -1;
	}

	// Get the "meta" object
	{
		JsonObject metaObj = {0};

		if (jsonFindObjByTag(pRoute, "meta", &metaObj) == 0) {
			if (jsonGetStringValue(&metaObj, "loc", &info.location) != 0) {
				fprintf(stderr, "ERROR: failed to get \"meta\" value!\n");
				return -1;
			}

			if (jsonGetStringValue(&metaObj, "dur", &info.duration) != 0) {
				fprintf(stderr, "ERROR: failed to get \"dur\" value!\n");
				return -1;
			}

			if (jsonGetStringValue(&metaObj, "dis", &info.distance) != 0) {
				fprintf(stderr, "ERROR: failed to get \"dis\" value!\n");
				return -1;
			}

			if (jsonGetStringValue(&metaObj, "ele", &info.elevation) != 0) {
				fprintf(stderr, "ERROR: failed to get \"ele\" value!\n");
				return -1;
			}

			if (jsonGetStringValue(&metaObj, "con", &info.contributor) != 0) {
				fprintf(stderr, "ERROR: failed to get \"con\" value!\n");
				return -1;
			}

			if (jsonGetStringValue(&metaObj, "tou", &info.toughness) != 0) {
				fprintf(stderr, "ERROR: failed to get \"tou\" value!\n");
				return -1;
			}

			if (jsonGetArrayValue(&metaObj, "cat", &info.categories) != 0) {
				fprintf(stderr, "ERROR: failed to get \"cat\" value!\n");
				return -1;
			}

			// Convert the duration to seconds
			{
			    int h, m, s;
			    sscanf(info.duration, "%u:%u:%u", &h, &m, &s);
			    info.time = (h * 3600) + (m * 60) + s;
			}
		}
	}

	// Get the "vimXXX" objects
	{
		JsonObject vimObj = {0};

		if (jsonFindObjByTag(pRoute, "vimMaster", &vimObj) == 0) {
			if (jsonGetStringValue(&vimObj, "file", &info.vimMaster) != 0) {
				fprintf(stderr, "ERROR: failed to get \"file\" value!\n");
				return -1;
			}
		}

		if (jsonFindObjByTag(pRoute, "vim1080", &vimObj) == 0) {
			if (jsonGetStringValue(&vimObj, "file", &info.vim1080) != 0) {
				fprintf(stderr, "ERROR: failed to get \"file\" value!\n");
				return -1;
			}
		}

		if (jsonFindObjByTag(pRoute, "vim720", &vimObj) == 0) {
			if (jsonGetStringValue(&vimObj, "file", &info.vim720) != 0) {
				fprintf(stderr, "ERROR: failed to get \"file\" value!\n");
				return -1;
			}
		}
	}

	// Get the "a" object
	{
		JsonObject aObj = {0};

		if (jsonFindObjByTag(pRoute, "a", &aObj) == 0) {
			if (jsonGetStringValue(&aObj, "file", &info.shiz) != 0) {
				fprintf(stderr, "ERROR: failed to get \"file\" value!\n");
				return -1;
			}
		}
	}

	if (applyMatchFilters(&info, pArgs) == 0) {
		RouteInfo *pRoute;

		if ((pRoute = malloc(sizeof (RouteInfo))) == NULL) {
			fprintf(stderr, "ERROR: failed to alloc route record!\n");
			return -1;
		}

		*pRoute = info;

		TAILQ_INSERT_TAIL(&pDb->routeList, pRoute, tqEntry);

		// Add entry to the DB
		pDb->numRoutes++;
	}

	return 0;
}

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

static void printCsvOutput(const RouteDB *pDb)
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
		printf("%s%s,", shizUrlPfx, pRoute->shiz);
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

static void printHttpOutput(const RouteDB *pDb)
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
		snprintf(link, sizeof (link), "%s%s", shizUrlPfx, pRoute->shiz);
		printHyperlinkCellValue(link);
		printf("            </tr>\n");
	}
	printf("        </table>\n");
	printf("    </body>\n");
	printf("</html>\n");
}

static int procMainObj(const JsonObject *pObj, const CmdArgs *pArgs)
{
	RouteDB routeDb = {0};
	char *result;
	char *data;

	TAILQ_INIT(&routeDb.routeList);

	if (jsonGetStringValue(pObj, "result", &result) != 0) {
		fprintf(stderr, "ERROR: failed to get \"result\" value!\n");
		return -1;
	}

	if (jsonGetStringValue(pObj, "prefix", &routeDb.mp4UrlPfx) != 0) {
		fprintf(stderr, "ERROR: failed to get \"prefix\" value!\n");
		return -1;
	}

	// Get the array of route objects
	if (jsonGetArrayValue(pObj, "data", &data) == 0) {
		JsonObject routeObj = {0};
		JsonObject *pRouteObj = &routeObj;
		const char *pData = data;
		size_t dataLen = strlen(data);

		// Process each route object in the "data" array
		while (jsonFindObject(pData, dataLen, pRouteObj) == 0) {
			// Process this route object and add new
			// entry into the Route DB...
			procRouteObj(&routeDb, pRouteObj, pArgs);

			pData = pRouteObj->end + 1;
			dataLen -= (pRouteObj->end - pRouteObj->start);
		}

		printf("numRoutes=%d\n", routeDb.numRoutes);

		// Create output file
		if (pArgs->outFmt == csv) {
		    printCsvOutput(&routeDb);
		} else if (pArgs->outFmt == html) {
		    printHttpOutput(&routeDb);
        }
	}

	return 0;
}

// Locate the BizarMobile.FulGaz_xxxx install direcotry
static char *getBizarMobilePath(void)
{
    char *userName;
    DIR *dirp;
    struct dirent *dp;
    char packagesDir[80];
    static char appInstDir[512];

    // Figure out our user name
    if ((userName = getenv("USER")) == NULL) {
        fprintf(stderr, "ERROR: can't determine user name (%s)\n", strerror(errno));
        return NULL;
    }

    // Full path to the Packages directory
    snprintf(packagesDir, sizeof (packagesDir), "/cygdrive/c/Users/%s/AppData/Local/Packages", userName);
    if ((dirp = opendir(packagesDir)) != NULL) {
        do {
            const char *bizarMobile = "BizarMobile.FulGaz";
            size_t len = strlen(bizarMobile);

            if ((dp = readdir(dirp)) != NULL) {
                if (strncmp(dp->d_name, bizarMobile, len) == 0) {
                    // Found it!
                    snprintf(appInstDir, sizeof (appInstDir), "%s/%s/", packagesDir, dp->d_name);
                    return appInstDir;
                }
            }
        } while (dp != NULL);
    } else {
        fprintf(stderr, "ERROR: can't open directory \"%s\" (%s)\n", packagesDir, strerror(errno));
    }

    return NULL;
}

static char *getFilePath(const char *appInstDir)
{
    static char filePath[1024];
    const char *fileName = "allrides_v4.json";
    struct stat stBuf = {0};

    // The path to the allrides_v4.json file depends
    // on the version of the app being used; e.g.
    // FulGaz version 4.2.x:  appInstDir/LocalState/allrides_v4.json
    // FulGaz version 4.50.x: appInstDir/LocalCache/Local/FulGaz/allrides_v4.json

    // Try 4.50.x ...
    snprintf(filePath, sizeof (filePath), "%sLocalCache/Local/FulGaz/%s", appInstDir, fileName);
    if ((stat(filePath, &stBuf) == 0) && S_ISREG(stBuf.st_mode))
        return filePath;

    // Try 4.2.x ...
    snprintf(filePath, sizeof (filePath), "%sLocalState/%s", appInstDir, fileName);
    if ((stat(filePath, &stBuf) == 0) && S_ISREG(stBuf.st_mode))
        return filePath;

    return NULL;
}

static const char *help =
        "SYNTAX:\n"
        "    whatsOnFulGaz [OPTIONS]\n"
        "\n"
        "    This command-line utility parses the JSON file that describes all the\n"
        "    available rides, and creates a CSV file or an HTML file with the list\n"
        "    of routes, that can be viewed with Excel or LibreOffice Calc (CSV) or\n"
        "    with Chrome or Edge (HTML).\n"
        "\n"
        "OPTIONS:\n"
        "    --contributor <name>\n"
        "        Only include rides submitted by the specified contributor. The name\n"
        "        match is case-insensitive and liberal: e.g. specifying \"mourier\"\n"
        "        will match all rides contributed by \"Marcelo Mourier\".\n"
        "    --country <name>\n"
        "        Only include rides from the specified country. The name match is \n"
        "        case-insensitive and liberal: e.g. specifying \"aus\" will match \n"
        "        all rides from \"Australia\" and from \"Austria\".\n"
        "    --get-video\n"
        "        Download the MP4 video file of the ride.\n"
        "    --help\n"
        "        Show this help and exit.\n"
        "    --output-format {csv|html}\n"
        "        Specifies the format of the output file.\n"
        "    --video-resolution {720|1080|4k}\n"
        "        Only include rides with video at the specified resolution.\n"
        "\n";

static int parseCmdArgs(int argc, char *argv[], CmdArgs *pArgs)
{
    int numArgs = argc - 1;

    for (int n = 1; n <= numArgs; n++) {
        const char *arg;
        const char *val;

        arg = argv[n];

        if (strcmp(arg, "--help") == 0) {
            fprintf(stdout, "%s\n", help);
            exit(0);
        }  else if (strcmp(arg, "--contributor") == 0) {
            pArgs->contributor = argv[++n];
        }  else if (strcmp(arg, "--country") == 0) {
            pArgs->country = argv[++n];
        }  else if (strcmp(arg, "--get-video") == 0) {
            pArgs->getVideo = 1;
        } else if (strcmp(arg, "--output-format") == 0) {
            val = argv[++n];
            if (strcmp(val, "csv") == 0) {
                pArgs->outFmt = csv;
            } else if (strcmp(val, "html") == 0) {
                pArgs->outFmt = html;
            } else {
                fprintf(stderr, "Invalid output format: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--video-resolution") == 0) {
            val = argv[++n];
            if (strcmp(val, "720") == 0) {
                pArgs->vidRes = res720p;
            } else if (strcmp(val, "1080") == 0) {
                pArgs->vidRes = res1080p;
            } else if (strcmp(val, "4k") == 0) {
                pArgs->vidRes = res4K;
            } else {
                fprintf(stderr, "Invalid video resolution: %s\n", val);
                return -1;
            }
        } else {
            fprintf(stderr, "Invalid option: %s\n", arg);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    CmdArgs cmdArgs = {0};
	char *appInstDir;
	char *filePath;
    int fd;
    struct stat stBuf = {0};
    char *data;
    size_t dataLen;
    JsonObject mainObj = {0};

    // Parse the command-line arguments
    if (parseCmdArgs(argc, argv, &cmdArgs) != 0) {
        fprintf(stderr, "Use --help for the list of supported options.\n\n");
        return -1;
    }

    // Figure out the install directory of the app
    if ((appInstDir = getBizarMobilePath()) == NULL) {
        fprintf(stderr, "ERROR: can't determine app's install directory\n");
        return -1;
    }

    // Figure out the full path to the allrides_v4.json file
    if ((filePath = getFilePath(appInstDir)) == NULL) {
        fprintf(stderr, "ERROR: can't get file path (%s)\n", strerror(errno));
        return -1;
    }

    printf("Found rides file: %s\n", filePath);

    if ((fd = open(filePath, O_RDONLY, 0)) < 0) {
        fprintf(stderr, "ERROR: can't open file \"%s\" (%s)\n", filePath, strerror(errno));
        return -1;
    }

    if (fstat(fd, &stBuf) != 0) {
        fprintf(stderr, "ERROR: can't get file size (%s)\n", strerror(errno));
        return -1;
    }

    dataLen = stBuf.st_size;

    if ((data = malloc(dataLen)) == NULL) {
        fprintf(stderr, "ERROR: can't alloc data buffer (%s)\n", strerror(errno));
        return -1;
    }

    if (read(fd, data, dataLen) != dataLen) {
        fprintf(stderr, "ERROR: can't read data (%s)\n", strerror(errno));
        return -1;
    }

    close(fd);

    // Locate the main JSON object
	if (jsonFindObject(data, dataLen, &mainObj) != 0) {
		fprintf(stderr, "ERROR: can't find main JSON object!\n");
		return -1;
	}

	//jsonDumpObject(&mainObj);

	// Process the main JSON object
	procMainObj(&mainObj, &cmdArgs);

    free(data);

    return 0;
}


