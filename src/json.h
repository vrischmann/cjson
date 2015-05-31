#pragma once

#if defined(_MSC_VER)

typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#elif defined(GNUC) || defined(__clang__)

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#endif

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

#define MAX_STRING_LENGTH 2048
#define MAX_CHILDREN_PER_NODE 2048

struct JSONString
{
    char data[MAX_STRING_LENGTH];
    size_t length;
};

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
