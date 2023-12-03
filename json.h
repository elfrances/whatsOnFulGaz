#pragma once

// A JSON object consists of text enclosed within matching
// curly braces.
typedef struct JsonObject {
    char *start;    // points to the left curly brace where the object starts
    char *end;      // points to the right curly brace where the object ends
} JsonObject;

// A JSON array consists of text enclosed within matching
// square brackets.
typedef struct JsonArray {
    char *start;    // points to the left square bracket where the array starts
    char *end;      // points to the right square bracket where the array ends
} JsonArray;

// Callback handler for the for-each iterator
typedef int (*JsonCbHdlr)(const JsonObject *, void *);

// Locate the specified tag within the given JSON object and
// return a pointer to its value: e.g.
//
//   { ..., <tag> : <value>, ... }
//
const char *jsonFindTag(const JsonObject *pObj, const char *tag);

// Format is: "<tag>":[<ent0>,<ent1>,...,<entN>]
int jsonGetArrayValue(const JsonObject *pObj, const char *tag, char **pVal);

// A JSON object consists of text enclosed within matching
// curly braces: e.g.
//
//    {"user':"John Doe","age":"35","gender":"male"}
//
int jsonFindObject(const char *data, size_t dataLen, JsonObject *pObj);

// Search the specified object for an embedded object with
// the given tag: e.g.
//
//   { ..., "info":{"make":"Honda","model":"Pilot","trim":"EX-L"}, ... }
//
int jsonFindObjByTag(const JsonObject *pObj, const char *tag, JsonObject *pEmbObj);

// Search the specified object for an array with the
// given tag: e.g.
//
//   { ..., "<tag>":[<ent0>,<ent1>,...,<entN>], ... }
//
int jsonFindArrayByTag(const JsonObject *pObj, const char *tag, JsonArray *pArray);

// Process each element in the specified array
int jsonArrayForEach(const JsonArray *pArray, JsonCbHdlr handler, void *arg);

// Format is: "<tag>":"<val>" where the value is a string
int jsonGetStringValue(const JsonObject *pObj, const char *tag, char **pVal);

// Format is: "<tag>":"<val>" where the value is a string representing
// the time in hh:mm:ss.
int jsonGetStrTimeValue(const JsonObject *pObj, const char *tag, time_t *pVal);

// Format is: "<tag>":"<val>" where the value is a string representing
// a double floating point number.
int jsonGetStrDoubleValue(const JsonObject *pObj, const char *tag, double *pVal);


