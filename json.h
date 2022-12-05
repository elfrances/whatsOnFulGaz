#pragma once

typedef struct JsonObject {
    char *start;    // points to the left curly brace where the record starts
    char *end;      // points to the right curly brace where the record ends
} JsonObject;

// Locate the specified tag within the given JSON object and
// return a pointer to its value: e.g.
//
//   { ..., <tag> : <value>, ... }
//
const char *jsonFindTag(const JsonObject *pObj, const char *tag);

// Format is: "<tag>":"<val>" where the value is a string
int jsonGetStringValue(const JsonObject *pObj, const char *tag, char **pVal);

// Format is: "<tag>":[<ent0>,<ent1>,...,<entN>]
int jsonGetArrayValue(const JsonObject *pObj, const char *tag, char **pVal);

// A JSON object consists of text enclosed within matching
// curly braces: e.g.
//
//    {"user':"John Doe","age":"35","gender":"male"}
//
int jsonFindObject(const char *data, size_t dataLen, JsonObject *pObj);

// Search the current object for an embedded object with
// the given tag: e.g.
//
//   { ..., "info":{"make":"Honda","model":"Pilot","trim":"EX-L"}, ...}
//
int jsonFindObjByTag(const JsonObject *pObj, const char *tag, JsonObject *pEmbObj);
