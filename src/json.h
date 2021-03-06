#pragma once

#include <stdint.h>

#if defined(BUILDING_JSON)
#define JSON_API __declspec(dllexport)
#else
#define JSON_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum JSONError
{
    ERR_NOERROR,
    ERR_EOF,
    ERR_INVALID_STRING,
    ERR_INVALID_TREE_SYNTAX,
    ERR_INVALID_OBJECT_SYNTAX,
    ERR_INVALID_BOOLEAN_SYNTAX,
    ERR_INVALID_FLOAT_SYNTAX,
    ERR_INVALID_UNICODE_LITERAL_SYNTAX,
    ERR_INVALID_ARRAY_SYNTAX,
    ERR_INVALID_NULL_SYNTAX,
    ERR_OUTPUT_BUFFER_TOO_SMALL,

    ERR_ITERATOR_INVALID_NODE,
    ERR_ITERATOR_INVALID_KEY_PTR,
    ERR_ITERATOR_INVALID_VALUE_PTR,
    ERR_ITERATOR_NO_MORE_ELEMENTS
};

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

JSON_API JSONNode* JSONCreateNode();
JSON_API void JSONFreeNode(JSONNode *tree);
JSON_API JSONError JSONParse(JSONNode *tree, const char *input, size_t inputLength);

typedef struct JSONIterator JSONIterator;

JSON_API JSONIterator* JSONCreateIterator(JSONNode *node);
JSON_API void JSONFreeIterator(JSONIterator *iter);
JSON_API JSONError JSONIteratorGetNext(JSONIterator *iter, JSONString **keyPtr, JSONNode **nodePtr);

#ifdef __cplusplus
}
#endif
