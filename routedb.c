#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routedb.h"

int rtDbInit(RouteDB *rtDb)
{
    memset(rtDb, 0, sizeof (RouteDB));
    TAILQ_INIT(&rtDb->routeList);
    return 0;
}

int rtDbAdd(RouteDB *rtDb, const RouteInfo *rtInfo)
{
    RouteInfo *pRoute;

    if ((pRoute = malloc(sizeof (RouteInfo))) == NULL) {
        fprintf(stderr, "ERROR: failed to alloc route record!\n");
        return -1;
    }

    *pRoute = *rtInfo;

    // Add entry to the DB
    TAILQ_INSERT_TAIL(&rtDb->routeList, pRoute, tqEntry);

    rtDb->numRoutes++;

    return 0;
}
