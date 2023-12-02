#pragma once

#include "args.h"
#include "routedb.h"

void printCsvOutput(const RouteDB *pDb, const CmdArgs *pArgs);
void printHttpOutput(const RouteDB *pDb, const CmdArgs *pArgs);
void printTextOutput(const RouteDB *pDb, const CmdArgs *pArgs);
