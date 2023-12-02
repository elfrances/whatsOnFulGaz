#pragma once

#include "args.h"

extern uint64_t totalContentLength;

extern char *fmtContentLength(uint64_t contentLength);
extern int urlDownload(const char *url, const char *outFile, const CmdArgs *pArgs);
