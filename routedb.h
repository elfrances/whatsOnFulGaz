#pragma once

#include <sys/queue.h>

typedef struct RouteInfo {
    TAILQ_ENTRY(RouteInfo) tqEntry;

    char *categories;   //
    char *contributor;  // Contributor
    char *description;  // Description
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

    // URL prefix for fetching the SHIZ file of a route
    char *shizUrlPfx;

    // List of routes
    TAILQ_HEAD(RouteList, RouteInfo) routeList;

    // Number of routes in the list
    int numRoutes;
} RouteDB;
