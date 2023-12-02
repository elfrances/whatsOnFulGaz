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

#include "args.h"
#include "download.h"
#include "json.h"
#include "output.h"
#include "routedb.h"

#if (OS_TYPE == OS_TYPE_MACOS)
#undef st_atim
#define st_atim st_atimespec
#endif

typedef struct AllRidesFile {
    char filePath[1024];
    time_t fileDate;
} AllRidesFile;

#define PROGRAM_VERSION "1.6"

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
 * 			"des":"A relatively easy 5% start, although on very hot south-facing slopes, with the toughest sections at more than 10% in the second half.  Filmed on the 2017 �tape du Tour.  Used on stage 18 of the 2017 Tour de France, the first rider over the summit at 2100m was Alexey Lutsenko (Astana) from Kazakhstan",
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
 * 		"t":"�tape du Tour 2017 - Col de Vars from Saint-Paul-sur-Ubaye "
 * }
 *
 * File structure:
 *
 * {
 * 	    "result":"success",
 *  	"prefix":"https://fulgaz.cachefly.net/file/fulgaz-videos/",
 *  	"data":[<rec1>,<rec2>,...<recN>]
 * }
 *
 */

static const char *help =
        "SYNTAX:\n"
        "    whatsOnFulGaz [OPTIONS]\n"
        "\n"
        "    whatsOnFulGaz is a command-line app that parses the JSON file that contains all\n"
        "    the available rides in the FulGaz library, and creates a CSV, HTML, or TXT\n"
        "    file with the list of routes. The CSV file can be viewed with a spreadsheet\n"
        "    app such as MS Excel, Google Sheets, or LibreOffice Calc, while the HTML file\n"
        "    can be viewed with a web browser app such as Chrome, Edge, or Safari.\n"
        "    The TXT file shows the route info in plain human-readable format.\n"
        "    The app has several filters that allow it to show only selected routes; e.g. by\n"
        "    contributor, country, maximum distance, etc.\n"
        "    Optionally, the app can download in the background the MP4 video file of all the\n"
        "    routes that matched the specified filters.\n"
        "\n"
        "OPTIONS:\n"
        "    --allrides-file <path>\n"
        "        Specifies the path to the JSON file that describes all the available\n"
        "        rides in the library.\n"
        "    --category <name>\n"
        "        Only include rides from the specified category. The name match is\n"
        "        case-insensitive and liberal: e.g. specifying \"hill\" will match \n"
        "        all rides contained in the category \"Hilly\". FulGaz supports the\n"
        "        following categories: Avatar, Easy, Hilly, IRONMAN, Long, Loop,\n"
        "        Mountain, New, Race, Sightseeing, Trails.\n"
        "    --contributor <name>\n"
        "        Only include rides submitted by the specified contributor. The name\n"
        "        match is case-insensitive and liberal: e.g. specifying \"mourier\"\n"
        "        will match all rides contributed by \"Marcelo Mourier\".\n"
        "    --country <name>\n"
        "        Only include rides from the specified country. The name match is \n"
        "        case-insensitive and liberal: e.g. specifying \"aus\" will match \n"
        "        all rides from \"Australia\" and from \"Austria\".\n"
        "    --download-folder <path>\n"
        "        Specifies the folder where the downloaded files are stored.\n"
        "    --download-progress\n"
        "        Show video download progress info.\n"
        "    --dry-run\n"
        "        Show what is going to be downloaded, without actually downloading\n"
        "        anything.\n"
        "    --get-shiz\n"
        "        Download the SHIZ control file of the ride.\n"
        "    --get-video {720|1080|4k}\n"
        "        Download the MP4 video file of the ride at the specified resolution.\n"
        "    --help\n"
        "        Show this help and exit.\n"
        "    --max-distance <value>\n"
        "        Only include rides with a distance (in Km's) up to the specified\n"
        "        value.\n"
        "    --max-duration <value>\n"
        "        Only include rides with a duration (in minutes) up to the specified\n"
        "        value.\n"        
        "    --max-elevation-gain <value>\n"
        "        Only include rides with an elevation gain (in meters) up to the \n"
        "        specified value.\n"
        "    --min-distance <value>\n"
        "        Only include rides with a distance (in Km's) above the specified\n"
        "        value.\n"
        "    --min-duration <value>\n"
        "        Only include rides with a duration (in minutes) above the specified\n"
        "        value.\n"
        "    --min-elevation-gain <value>\n"
        "        Only include rides with an elevation gain (in meters) above the \n"
        "        specified value.\n"
        "    --output-format {csv|html|text}\n"
        "        Specifies the format of the output file with the list of routes.\n"
        "        If omitted, the plain text format is used by default.\n"
        "    --province <name>\n"
        "        Only include rides from the specified province or state in the\n"
        "        specified country. The name match is case-insensitive and liberal:\n"
        "        e.g. specifying \"cali\" will match all rides from California, USA.\n"
        "    --title <name>\n"
        "        Only include rides that have <name> in their title. The name\n"
        "        match is case-insensitive and liberal: e.g. specifying \"gavia\"\n"
        "        will match the rides \"Passo di Gavia\", \"Passo di Gavia Sweet\n"
        "        Spot\", and \"Passo di Gavia from Ponte di Legno\".\n"
        "    --version\n"
        "        Show program's version info and exit.\n"
        "\n"
        "NOTES:\n"
        "    Running the tool under Windows/Cygwin the drive letters are replaced by\n"
        "    their equivalent cygdrive: e.g. the path \"C:\\Users\\Marcelo\\Documents\"\n"
        "    becomes \"/cygdrive/c/Users/Marcelo/Documents\".\n"
        "\n"
        "BUGS:\n"
        "    Report bugs and enhancement requests to: marcelo_mourier@yahoo.com\n";

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
        } else if (strcmp(arg, "--version") == 0) {
            fprintf(stdout, "Program version %s built on %s %s\n", PROGRAM_VERSION, __DATE__, __TIME__);
            exit(0);
        } else if (strcmp(arg, "--allrides-file") == 0) {
            pArgs->inFile = argv[++n];
        } else if (strcmp(arg, "--category") == 0) {
            pArgs->category = argv[++n];                        
        } else if (strcmp(arg, "--contributor") == 0) {
            pArgs->contributor = argv[++n];            
        } else if (strcmp(arg, "--country") == 0) {
            pArgs->country = argv[++n];
        } else if (strcmp(arg, "--download-folder") == 0) {
            pArgs->dlFolder = argv[++n];
        } else if (strcmp(arg, "--download-progress") == 0) {
            pArgs->dlProg = 1;
        } else if (strcmp(arg, "--dry-run") == 0) {
            pArgs->dryRun = 1;
        } else if (strcmp(arg, "--get-shiz") == 0) {
            pArgs->getShiz = 1;
        } else if (strcmp(arg, "--get-video") == 0) {
            val = argv[++n];
            if (strcmp(val, "720") == 0) {
                pArgs->getVideo = res720p;
            } else if (strcmp(val, "1080") == 0) {
                pArgs->getVideo = res1080p;
            } else if ((strcmp(val, "4k") == 0) ||
                       (strcmp(val, "4K") == 0)) {
                pArgs->getVideo = res4K;
            } else {
                fprintf(stderr, "Invalid video resolution: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--max-distance") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->maxDistance) != 1) {
                fprintf(stderr, "Invalid max distance value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--max-duration") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->maxDuration) != 1) {
                fprintf(stderr, "Invalid max duration value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--max-elevation-gain") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->maxElevGain) != 1) {
                fprintf(stderr, "Invalid max elevation gain value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--min-distance") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->minDistance) != 1) {
                fprintf(stderr, "Invalid min distance value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--min-duration") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->minDuration) != 1) {
                fprintf(stderr, "Invalid min duration value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--min-elevation-gain") == 0) {
            val = argv[++n];
            if (sscanf(val, "%d", &pArgs->minElevGain) != 1) {
                fprintf(stderr, "Invalid min elevation gain value: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--output-format") == 0) {
            val = argv[++n];
            if (strcmp(val, "csv") == 0) {
                pArgs->outFmt = csv;
            } else if (strcmp(val, "html") == 0) {
                pArgs->outFmt = html;
            } else if (strcmp(val, "text") == 0) {
                pArgs->outFmt = text;
            } else {
                fprintf(stderr, "Invalid output format: %s\n", val);
                return -1;
            }
        } else if (strcmp(arg, "--province") == 0) {
            pArgs->province = argv[++n];
        } else if (strcmp(arg, "--title") == 0) {
            pArgs->title = argv[++n];
        } else {
            fprintf(stderr, "Invalid option: %s\n", arg);
            return -1;
        }
    }

    // Sanity check the min/max values
    if ((pArgs->maxDistance != 0) && (pArgs->minDistance > pArgs->maxDistance)) {
        fprintf(stderr, "The minimum distance can't be greater than the maximum distance\n");
        return -1;
    }
    if ((pArgs->maxDuration != 0) && (pArgs->minDuration > pArgs->maxDuration)) {
        fprintf(stderr, "The minimum duration can't be greater than the maximum duration\n");
        return -1;
    }
    if ((pArgs->maxElevGain != 0) && (pArgs->minElevGain > pArgs->maxElevGain)) {
        fprintf(stderr, "The minimum elevation gain can't be greater than the maximum elevation gain\n");
        return -1;
    }

    if (pArgs->inFile != NULL) {
        // Make sure the allrides file exists
        struct stat stBuf = {0};
        if ((stat(pArgs->inFile, &stBuf) != 0) || !S_ISREG(stBuf.st_mode)) {
            fprintf(stderr, "Invalid allrides folder: %s\n", pArgs->inFile);
            return -1;
        }
    }

    if (pArgs->dlFolder != NULL) {
        // Make sure the download folder exists
        struct stat stBuf = {0};
        if ((stat(pArgs->dlFolder, &stBuf) != 0) || !S_ISDIR(stBuf.st_mode)) {
            fprintf(stderr, "Invalid download folder: %s\n", pArgs->dlFolder);
            return -1;
        }
    } else {
        // Download to the current directory
        pArgs->dlFolder = ".";
    }

    if ((pArgs->outFmt == undef) && (pArgs->getVideo == none) && (pArgs->getShiz == 0)) {
        // Omitting the output file format is only
        // allowed when downloading the video or
        // the shiz files.
        //fprintf(stderr, "INFO: Output file format not specified; using plain text by default.\n");
        pArgs->outFmt = text;
    }

    return 0;
}

static OsTyp getOsType(void)
{
    OsTyp osType = OS_TYPE;    // defined in the Makefile

    if (osType == unk) {
        // Running on macOS Darwin the Makefile ends up
        // with OS_TYPE=0 so try to fix it here...
        if (getenv("LOGNAME") != NULL)
            osType = macOS;
    }

    return osType;
}

// Locate the BizarMobile.FulGaz_xxxx install direcotry
static char *getBizarMobilePath(OsTyp osTyp)
{
    char *userName = NULL;
    static char appInstDir[512];

    // Figure out our user name
    if (osTyp == macOS) {
        userName = getenv("LOGNAME");
    } else if (osTyp == cygwin) {
        userName = getenv("USERNAME");
    } else if (osTyp == gnuLinux) {
        userName = getenv("USER");
    }
    if (userName == NULL) {
        fprintf(stderr, "ERROR: can't determine user name (%s)\n", strerror(errno));
        return NULL;
    }

    if (osTyp == macOS) {
        snprintf(appInstDir, sizeof (appInstDir), "/Users/%s/Library/Containers/com.bizarmobile.fulgaz/", userName);
        return appInstDir;
    } else if (osTyp == cygwin) {
        char packagesDir[80];
        DIR *dirp;

        // Full path to the Packages directory
        snprintf(packagesDir, sizeof (packagesDir), "/cygdrive/c/Users/%s/AppData/Local/Packages", userName);
        if ((dirp = opendir(packagesDir)) != NULL) {
            struct dirent *dp;
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
    }

    return NULL;
}

static int getFilePath(const char *appInstDir, OsTyp osTyp, AllRidesFile *allRides)
{
    const char *fileName = "allrides_v4.json";
    struct stat stBuf = {0};

    allRides->filePath[0] = '\0';
    allRides->fileDate = 0;

    // The path to the allrides_v4.json file depends on
    // the OS and the version of the app being used...

    if (osTyp == macOS) {
        // macOS app version 4.3.x: appInstDir/Data/Library/Application Support/FulGaz/allrides_v4.json
        snprintf(allRides->filePath, sizeof (allRides->filePath), "%sData/Library/Application Support/FulGaz/%s", appInstDir, fileName);
        if ((stat(allRides->filePath, &stBuf) == 0) && S_ISREG(stBuf.st_mode)) {
            allRides->fileDate = stBuf.st_atim.tv_sec;
            return 0;
        }
        return 0;
    } else {
        // Windows app version 4.2.x:  appInstDir/LocalState/allrides_v4.json
        // Windows app version 4.50.x and 5.0.x: appInstDir/LocalCache/Local/FulGaz/allrides_v4.json

        // Try 4.50.x and 5.0.x ...
        snprintf(allRides->filePath, sizeof (allRides->filePath), "%sLocalCache/Local/FulGaz/%s", appInstDir, fileName);
        if ((stat(allRides->filePath, &stBuf) == 0) && S_ISREG(stBuf.st_mode)) {
            allRides->fileDate = stBuf.st_atim.tv_sec;
            return 0;
        }

        // Try 4.2.x ...
        snprintf(allRides->filePath, sizeof (allRides->filePath), "%sLocalState/%s", appInstDir, fileName);
        if ((stat(allRides->filePath, &stBuf) == 0) && S_ISREG(stBuf.st_mode)) {
            allRides->fileDate = stBuf.st_atim.tv_sec;
            return 0;
        }
    }

    return -1;
}

// Cygwin doesn't have this one
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

static int applyMatchFilters(const RouteInfo *pInfo, const CmdArgs *pArgs)
{
    if ((pArgs->category != NULL) && (stristr(pInfo->categories, pArgs->category) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->contributor != NULL) && (stristr(pInfo->contributor, pArgs->contributor) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->country != NULL) && (stristr(pInfo->location, pArgs->country) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->province != NULL) && (stristr(pInfo->location, pArgs->province) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    if ((pArgs->title != NULL) && (stristr(pInfo->title, pArgs->title) == NULL)) {
        // Ignore this ride...
        return -1;
    }
    {
        int distance = atoi(pInfo->distance);
        if ((pArgs->maxDistance != 0) && (distance > pArgs->maxDistance)) {
            // Ignore this ride...
            return -1;
        }
        if ((pArgs->minDistance != 0) && (distance < pArgs->minDistance)) {
            // Ignore this ride...
            return -1;
        }
    }
    {
        int duration = pInfo->time / 60;
        if ((pArgs->maxDuration != 0) && (duration > pArgs->maxDuration)) {
            // Ignore this ride...
            return -1;
        }
        if ((pArgs->minDuration != 0) && (duration < pArgs->minDuration)) {
            // Ignore this ride...
            return -1;
        }
    }
    {
        int elevation = atoi(pInfo->elevation);
        if ((pArgs->maxElevGain != 0) && (elevation > pArgs->maxElevGain)) {
            // Ignore this ride...
            return -1;
        }
        if ((pArgs->minElevGain != 0) && (elevation < pArgs->minElevGain)) {
            // Ignore this ride...
            return -1;
        }
    }

    return 0;
}

static void strShiftLeft(char *str, size_t count)
{
    size_t len = strlen(str) + 1;
    memmove(str, (str+count), (len - count));
}

static void cleanUpDescription(char *desc)
{
    char *p;

    p = desc;
    while ((p = strstr(p, "\\\"")) != NULL) {
        strShiftLeft(p, 1);
    }

    p = desc;
    while ((p = strstr(p, "\\n")) != NULL) {
        strShiftLeft(p, 2);
    }
}

typedef struct CbInfo {
    RouteDB *routeDb;
    const CmdArgs *cmdArgs;
} CbInfo;

static int procRouteObj(const JsonObject *pRoute, void *arg)
{
    CbInfo *pInfo = arg;
    RouteDB *pDb = pInfo->routeDb;
    const CmdArgs *pArgs = pInfo->cmdArgs;
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

			// Duration
			if (jsonGetStringValue(&metaObj, "dur", &info.duration) != 0) {
				fprintf(stderr, "ERROR: failed to get \"dur\" value!\n");
				return -1;
			}

			// Distance
            if (jsonGetStringValue(&metaObj, "dis", &info.distance) != 0) {
                fprintf(stderr, "ERROR: failed to get \"dis\" value!\n");
                return -1;
            }

            // Description
            if (jsonGetStringValue(&metaObj, "des", &info.description) != 0) {
                fprintf(stderr, "ERROR: failed to get \"des\" value!\n");
                return -1;
            }

            // Elevation gain
			if (jsonGetStringValue(&metaObj, "ele", &info.elevation) != 0) {
				fprintf(stderr, "ERROR: failed to get \"ele\" value!\n");
				return -1;
			}

			// Contributor
			if (jsonGetStringValue(&metaObj, "con", &info.contributor) != 0) {
				fprintf(stderr, "ERROR: failed to get \"con\" value!\n");
				return -1;
			}

			// Toughness
			if (jsonGetStringValue(&metaObj, "tou", &info.toughness) != 0) {
				fprintf(stderr, "ERROR: failed to get \"tou\" value!\n");
				return -1;
			}

			// Categories
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
		    // SHIZ control file
			if (jsonGetStringValue(&aObj, "file", &info.shiz) != 0) {
				fprintf(stderr, "ERROR: failed to get \"file\" value!\n");
				return -1;
			}
		}
	}

	if (applyMatchFilters(&info, pArgs) == 0) {
		RouteInfo *pRoute;

		// Clean up the description string
		cleanUpDescription(info.description);

		if ((pRoute = malloc(sizeof (RouteInfo))) == NULL) {
			fprintf(stderr, "ERROR: failed to alloc route record!\n");
			return -1;
		}

		*pRoute = info;

		// Add entry to the DB
		TAILQ_INSERT_TAIL(&pDb->routeList, pRoute, tqEntry);

		pDb->numRoutes++;
	}

	return 0;
}

static void getShizFiles(const RouteDB *pDb, const CmdArgs *pArgs)
{
    RouteInfo *pRoute;

    TAILQ_FOREACH(pRoute, &pDb->routeList, tqEntry) {
        char url[256];
        snprintf(url, sizeof (url), "%s%s", pDb->shizUrlPfx, pRoute->shiz);
        urlDownload(url, NULL, pArgs);
    }
}

static void getVideoFiles(const RouteDB *pDb, const CmdArgs *pArgs)
{
    RouteInfo *pRoute;

    TAILQ_FOREACH(pRoute, &pDb->routeList, tqEntry) {
        char url[256];
        if (pArgs->getVideo == res720p) {
            snprintf(url, sizeof (url), "%s%s", pDb->mp4UrlPfx, pRoute->vim720);
        } else if (pArgs->getVideo == res1080p) {
            snprintf(url, sizeof (url), "%s%s", pDb->mp4UrlPfx, pRoute->vim1080);
        } else {
            snprintf(url, sizeof (url), "%s%s", pDb->mp4UrlPfx, pRoute->vimMaster);
        }
        urlDownload(url, NULL, pArgs);
    }
}

static int procMainObj(const JsonObject *pObj, const CmdArgs *pArgs)
{
	RouteDB routeDb = {0};
	char *result;
	JsonArray data = {0};

	TAILQ_INIT(&routeDb.routeList);
	routeDb.shizUrlPfx = "https://assets.fulgaz.com/";

	if (jsonGetStringValue(pObj, "result", &result) != 0) {
		fprintf(stderr, "ERROR: failed to get \"result\" value!\n");
		return -1;
	}

	if (jsonGetStringValue(pObj, "prefix", &routeDb.mp4UrlPfx) != 0) {
		fprintf(stderr, "ERROR: failed to get \"prefix\" value!\n");
		return -1;
	}

	// Get the "data":[] array which contains all
	// the route objects in the library.
	if (jsonFindArrayByTag(pObj, "data", &data) == 0) {
		// Process each route object in the "data" array ...
	    CbInfo cbInfo = { .routeDb = &routeDb, .cmdArgs = pArgs };
	    if (jsonArrayForEach(&data, procRouteObj, &cbInfo) != 0) {
	        // Error already printed
	        return -1;
	    }

		//printf("numRoutes=%d\n", routeDb.numRoutes);

		// Create output file
		if (pArgs->outFmt == csv) {
		    printCsvOutput(&routeDb);
        } else if (pArgs->outFmt == html) {
            printHttpOutput(&routeDb);
        } else if (pArgs->outFmt == text) {
            printTextOutput(&routeDb);
        }

        // If requested, download the SHIZ control files
        if (pArgs->getShiz) {
            getShizFiles(&routeDb, pArgs);
        }

        // If requested, download the MP4 video files
        if (pArgs->getVideo) {
            getVideoFiles(&routeDb, pArgs);
        }
	}

	return 0;
}

int main(int argc, char *argv[])
{
    CmdArgs cmdArgs = {0};
    OsTyp osTyp = unk;
    AllRidesFile allRides = {0};
	struct InFile {
        const char *filePath;
        char *data;
        size_t dataLen;
	} inFile = {0};
    JsonObject mainObj = {0};

    // Parse the command-line arguments
    if (parseCmdArgs(argc, argv, &cmdArgs) != 0) {
        fprintf(stderr, "Use --help for the list of supported options.\n\n");
        return -1;
    }

    // Figure out the OS type
    osTyp = getOsType();

    if ((inFile.filePath = cmdArgs.inFile) == NULL) {
        char *appInstDir;

        // Figure out the install directory of the app
        if ((appInstDir = getBizarMobilePath(osTyp)) == NULL) {
            fprintf(stderr, "ERROR: can't determine app's install directory\n");
            return -1;
        }

        // Figure out the full path to the allrides_v4.json file
        if (getFilePath(appInstDir, osTyp, &allRides) != 0) {
            fprintf(stderr, "ERROR: can't find rides directory file (%s)\n", strerror(errno));
            return -1;
        }

        inFile.filePath = allRides.filePath;

        {
            struct tm brkDwnTime = {0};
            char dateAndTimeBuf[128];
            strftime(dateAndTimeBuf, sizeof (dateAndTimeBuf), "%Y-%m-%dT%H:%M:%S", gmtime_r(&allRides.fileDate, &brkDwnTime));
            printf("Found rides directory file at: \"%s\" and dated: %s ...\n", inFile.filePath, dateAndTimeBuf);
        }
    }

    //printf("Found rides file: %s\n", filePath);

    // Read in the entire file
    {
        int fd;
        struct stat stBuf = {0};

        if ((fd = open(inFile.filePath, O_RDONLY, 0)) < 0) {
            fprintf(stderr, "ERROR: can't open file \"%s\" (%s)\n", inFile.filePath, strerror(errno));
            return -1;
        }

        if (fstat(fd, &stBuf) != 0) {
            fprintf(stderr, "ERROR: can't get file size (%s)\n", strerror(errno));
            return -1;
        }

        inFile.dataLen = stBuf.st_size;

        if ((inFile.data = malloc(inFile.dataLen)) == NULL) {
            fprintf(stderr, "ERROR: can't alloc data buffer (%s)\n", strerror(errno));
            return -1;
        }

        if (read(fd, inFile.data, inFile.dataLen) != inFile.dataLen) {
            fprintf(stderr, "ERROR: can't read data (%s)\n", strerror(errno));
            return -1;
        }

        close(fd);
    }

    // Locate the main JSON object
	if (jsonFindObject(inFile.data, inFile.dataLen, &mainObj) != 0) {
		fprintf(stderr, "ERROR: can't find main JSON object!\n");
		return -1;
	}

	//jsonDumpObject(&mainObj);

	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        fprintf(stderr, "ERROR: can't init CURL library!\n");
        return -1;
	}

	// Process the main JSON object
	procMainObj(&mainObj, &cmdArgs);

	curl_global_cleanup();

    free(inFile.data);

    return 0;
}


