#pragma once

typedef enum OutFmt {
    undef = 0,
    csv = 1,
    html = 2,
    text = 3,
} OutFmt;

typedef enum VidRes {
    none = 0,
    res720p = 1,
    res1080p = 2,
    res4K = 3,
} VidRes;

#define OS_TYPE_UNDEF  0
#define OS_TYPE_MACOS  1
#define OS_TYPE_CYGWIN 2
#define OS_TYPE_LINUX  3

typedef enum OsTyp {
    unk = OS_TYPE_UNDEF,
    macOS = OS_TYPE_MACOS,
    cygwin = OS_TYPE_CYGWIN,
    gnuLinux = OS_TYPE_LINUX,
} OsTyp;

typedef struct CmdArgs {
    const char *inFile;
    const char *category;
    const char *contributor;
    const char *country;
    const char *province;
    const char *title;
    const char *dlFolder;
    OutFmt outFmt;
    VidRes getVideo;
    int getShiz;
    int dlProg;
    int dryRun;
    int maxDistance;
    int maxDuration;
    int maxElevGain;
    int minDistance;
    int minDuration;
    int minElevGain;
} CmdArgs;
