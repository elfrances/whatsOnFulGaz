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

static size_t jsonGetObjDataLen(const JsonObject *pObj)
{
    return (pObj->end - pObj->start + 1);
}
#endif

void jsonDumpObject(const JsonObject *pObj)
{
    for (const char *p = pObj->start; p <= pObj->end; p++) {
        fputc(*p, stdout);
    }
    fputc('\n', stdout);
    fflush(stdout);
}

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
            for (const char *p = leftBracket; p <= pObj->end; p++) {
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
    const char *start = data;
    const char *end = data + dataLen - 1;

    // Locate the left curly brace, within the
    // available data block...
    while (start < end) {
        if (*start == '{') {
            pObj->start = (char *) start;
            break;
        } else {
            start++;
        }
    }

    if (start != end) {
        int level = 0;

        // Locate the matching right curly brace which
        // terminates the JSON object.
        for (char *p = pObj->start; p <= end; p++) {
            if (*p == '{') {
                level++;
                //printf("%s: p=%p level=%u\n", __func__, p, level);
            } else if (*p == '}') {
                if (level > 0) {
                    level--;
                    //printf("%s: p=%p level=%u\n", __func__, p, level);
                } else {
                    printf("%s: SPONG! Malformed JSON object!\n", __func__);
                    break;
                }
                if (level == 0) {
                    pObj->end = p;
                    return 0;
                }
            }
        }
    }

    pObj->start = NULL;
    pObj->end = NULL;

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
int jsonFindArrayByTag(const JsonObject *pObj, const char *tag, JsonObject *pArray)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
        // Locate the left square bracket
        char *leftBracket = strchr(val, '[');
        if (leftBracket != NULL) {
            // Locate the matching right square bracket
            int level = 0;
            for (char *p = leftBracket; p <= pObj->end; p++) {
                if (*p == '[') {
                    level++;
                } else if (*p == ']') {
                    if (--level == 0) {
                        char *rightBracket = p;
                        pArray->start = leftBracket;
                        pArray->end = rightBracket;
                        //jsonDumpObject(pArray);
                        return 0;
                    }
                }
            }
        }
    }

    return -1;
}

// Process each element object in the specified array
int jsonArrayForEach(const JsonObject *pArray, JsonCbHdlr handler, void *arg)
{
    const char *data = pArray->start;
    size_t dataLen = pArray->end - data + 1;
    JsonObject trkptObj;

    //printf("%s: start=%p end=%p dataLen=%zu\n", __func__, pArray->start, pArray->end, dataLen);

    while (data < pArray->end) {
        if (jsonFindObject(data, dataLen, &trkptObj) == 0) {
            // Paranoia?
            if ((trkptObj.start > pArray->end) || (trkptObj.end > pArray->end)) {
                printf("%s: SPONG! Object is outside the data range: trkptStart=%p trkptEnd=%p dataLen=%zu\n", __func__, trkptObj.start, trkptObj.end, dataLen);
                jsonDumpObject(&trkptObj);
                break;
            }

            // Call the handler
            if (handler(&trkptObj, arg) != 0) {
                // Oops!
                return -1;
            }

            data = trkptObj.end + 1;
            dataLen = pArray->end - data + 1;
        } else {
            //printf("%s: No more objects in the array! data=%p dataLen=%zu\n", __func__, data, dataLen);
            break;
        }
    }

    return 0;
}

// Format is: "<tag>":"<val>" where the value is a string
int jsonGetStringValue(const JsonObject *pObj, const char *tag, char **pVal)
{
    const char *val;

    if ((val = jsonFindTag(pObj, tag)) != NULL) {
        const char *openQuotes = strchr(val, '"');
        if (openQuotes != NULL) {
            for (const char *p = (openQuotes+1); p <= pObj->end; p++) {
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

// Format is: "<tag>":"<val>" where the value is a string representing
// the time in hh:mm:ss.
int jsonGetStrTimeValue(const JsonObject *pObj, const char *tag, time_t *pVal)
{
    char *value = NULL;
    unsigned int hr, min, sec;
    int s = 0;

    if (jsonGetStringValue(pObj, tag, &value) != 0)
        return -1;

    if (sscanf(value, "%u:%u:%u", &hr, &min, &sec) == 3) {
        *pVal = (hr * 3600) + (min * 60) + sec;
    } else {
        s = -1;
    }

    free(value);

    return s;
}

// Format is: "<tag>":"<val>" where the value is a string representing
// a double floating point number.
int jsonGetStrDoubleValue(const JsonObject *pObj, const char *tag, double *pVal)
{
    char *value = NULL;
    int s = 0;

    if (jsonGetStringValue(pObj, tag, &value) != 0)
        return -1;

    if (sscanf(value, "%le", pVal) != 1) {
        s = -1;
    }

    free(value);

    return s;
}

// Format is: "<tag>":<double> where the value is a double float number.
int jsonGetDoubleValue(const JsonObject *pObj, const char *tag, double *pVal)
{
    const char *value = NULL;
    int s = 0;

    if ((value = jsonFindTag(pObj, tag)) == NULL)
        return -1;

    if (sscanf(value, "%le", pVal) != 1) {
        s = -1;
    }

    return s;
}
