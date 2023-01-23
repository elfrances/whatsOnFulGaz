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

typedef enum OsTyp {
    unk = 0,
    macOS = 1,
    windows = 2,
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
    int maxDistance;
    int maxElevGain;
    int minDuration;
    int maxDuration;
} CmdArgs;
