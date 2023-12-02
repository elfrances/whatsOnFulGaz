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

#include "json.h"

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
const char *jsonFindTag(const JsonObject *pObj, const char *tag)
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
int jsonGetStringValue(const JsonObject *pObj, const char *tag, char **pVal)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
        const char *openQuotes = strchr(val, '"');
        if (openQuotes != NULL) {
            for (const char *p = (openQuotes+1); p != pObj->end; p++) {
                if (*p == '"') {
                    if (*(p - 1) == '\\') {
                        // This is an escaped double quote that
                        // is part of the string value...
                        continue;
                    } else {
                        const char *endQuotes = p;
                        char *str = stringify((openQuotes+1), (endQuotes - 1));
                        if (str != NULL) {
                            *pVal = str;
                            return 0;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

// Format is: "<tag>":[<ent0>,<ent1>,...,<entN>]
int jsonGetArrayValue(const JsonObject *pObj, const char *tag, char **pVal)
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
int jsonFindObject(const char *data, size_t dataLen, JsonObject *pObj)
{
    pObj->start = NULL;
    pObj->end = NULL;

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
                if (--level == 0) {
                    pObj->end = p;
                    return 0;
                }
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
int jsonFindObjByTag(const JsonObject *pObj, const char *tag, JsonObject *pEmbObj)
{
    const char *lbl;

    if ((lbl = jsonFindTag(pObj, tag)) != NULL) {
        size_t dataLen = (pObj->end - lbl);
        return jsonFindObject(lbl, dataLen, pEmbObj);
    }

    return -1;
}

// Search the specified object for an array with the
// given tag: e.g.
//
//   { ..., "<tag>":[<ent0>,<ent1>,...,<entN>], ... }
//
int jsonFindArrayByTag(const JsonObject *pObj, const char *tag, JsonArray *pArray)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
        // Locate the left square bracket
        char *leftBracket = strchr(val, '[');
        if (leftBracket != NULL) {
            // Locate the matching right square bracket
            int level = 0;
            for (char *p = leftBracket; p != pObj->end; p++) {
                if (*p == '[') {
                    level++;
                } else if (*p == ']') {
                    if (--level == 0) {
                        char *rightBracket = p;
                        pArray->start = leftBracket;
                        pArray->end = rightBracket;
                        return 0;
                    }
                }
            }
        }
    }

    return -1;
}

// Process each element object in the specified array
int jsonArrayForEach(const JsonArray *pArray, JsonCbHdlr handler, void *arg)
{
    const char *data = pArray->start + 1;   // skip the '[' character
    size_t dataLen = pArray->end - data;
    JsonObject trkptObj;

    while (data < pArray->end) {
        if (jsonFindObject(data, dataLen, &trkptObj) == 0) {
            // Call the handler
            if (handler(&trkptObj, arg) != 0) {
                // Oops!
                return -1;
            }
            data = trkptObj.end + 1;
            dataLen -= (trkptObj.end - trkptObj.start + 1);
        } else {
            break;
        }
    }

    return 0;
}
