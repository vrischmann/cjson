#pragma once

#include "types.h"
#include "errors.h"

#if defined(BUILDING_JSON)
#define JSON_API __declspec(dllexport)
#else
#define JSON_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum JSONNodeType
{
    UNKNOWN,
    OBJECT_NODE,
    ARRAY_NODE,
    STRING_NODE,
    DOUBLE_NODE,
    INTEGER_NODE,
    BOOLEAN_NODE,
    NULL_NODE
};

JSON_API const char* JSONNodeTypeToString(JSONNodeType type);

#define MAX_KEYS 16
#define MAX_VALUES 16

#define MAX_CHILDREN_PER_NODE 2048

typedef struct JSONString JSONString;

JSON_API char* JSONStringGetData(JSONString *string);

typedef struct JSONNode JSONNode;

JSON_API JSONNodeType JSONGetNodeType(JSONNode *node);
JSON_API JSONString* JSONNodeGetString(JSONNode *node);
JSON_API bool JSONNodeGetBool(JSONNode *node);
JSON_API int64_t JSONNodeGetInteger(JSONNode *node);
JSON_API double JSONNodeGetDouble(JSONNode *node);

JSON_API JSONNode* JSONCreateTree();
JSON_API JSONError JSONParseTree(JSONNode *tree, const char *input, size_t inputLength);
JSON_API void JSONFreeTree(JSONNode *tree); // TODO

typedef struct JSONIterator JSONIterator;

JSON_API JSONIterator* JSONCreateIterator(JSONNode *node);
JSON_API JSONError JSONIteratorGetNext(JSONIterator *iter, JSONString **keyPtr, JSONNode **nodePtr);

#ifdef __cplusplus
}
#endif
