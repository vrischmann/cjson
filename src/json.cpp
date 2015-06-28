#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "json.h"

//
// JSONString private API
//

struct JSONString
{
    Buffer *buffer;
};

void initJSONString(JSONString *string)
{
    string->buffer = newBuffer();
}

JSONString* newJSONString()
{
    JSONString *ptr = (JSONString*) malloc(sizeof(JSONString));
    initJSONString(ptr);
    return ptr;
}

// NOTE(vincent): this does not *free* the string !!
// It only cleans/frees all fields of the string
void cleanJSONString(JSONString *string)
{
    freeBuffer(string->buffer);
}

JSONError setJSONStringData(JSONString *string, char *buffer, size_t length)
{
    return putArrayToBuffer(string->buffer, buffer, length);
}

JSONError setJSONStringData(JSONString *string, Buffer *buffer)
{
    return copyBuffer(string->buffer, buffer);
}

//
// JSONString public API
//

JSON_API char* JSONStringGetData(JSONString *string)
{
    return getDataFromBuffer(string->buffer);
}

//
// JSONStringArray private API
//

struct JSONStringArray
{
    JSONString *underlying;
    size_t capacity;
    size_t index;
};

void freeJSONStringArray(JSONStringArray *array)
{
    for (size_t i = 0; i < array->index; i++)
    {
        cleanJSONString(&array->underlying[i]);
    }
    free(array->underlying);
}

JSONStringArray* newJSONStringArray(size_t capacity)
{
    JSONStringArray *array = (JSONStringArray *) malloc(sizeof(JSONStringArray));
    array->capacity = capacity;
    array->underlying = (JSONString *) malloc(sizeof(JSONString) * capacity);
    memset(array->underlying, 0, sizeof(JSONString) * capacity);
    array->index = 0;

    return array;
}

JSONString* newJSONStringFromArray(JSONStringArray *array)
{
    JSONString *ptr = array->underlying + array->index;
    array->index++;

    initJSONString(ptr);

    return ptr;
}

//
// JSONNode API
//

struct JSONNode
{
    JSONNodeType type;

    JSONStringArray *keys;
    JSONNode *values;
    size_t length;

    union
    {
        JSONString *stringValue;
        bool booleanValue;
        int64_t intValue;
        double doubleValue;
    };
};

JSON_API const char* JSONNodeTypeToString(JSONNodeType type)
{
    switch (type)
    {
        case OBJECT_NODE:  return "OBJECT_NODE";
        case ARRAY_NODE:   return "ARRAY_NODE";
        case STRING_NODE:  return "STRING_NODE";
        case DOUBLE_NODE:  return "DOUBLE_NODE";
        case INTEGER_NODE: return "INTEGER_NODE";
        case BOOLEAN_NODE: return "BOOLEAN_NODE";
        case NULL_NODE:    return "NULL_NODE";
        default:
           return "UNKNOWN";
    }
}

struct parseContext
{
    const char *input;
    size_t *index;
    size_t inputLength;
};

// forward declare because of parseKeyValuePair
JSONError parseObjectNode(JSONNode *node, parseContext *ctx, bool *parsedSomething);
JSONError parseArrayNode(JSONNode *node, parseContext *ctx, bool *parsedSomething);

JSONError consumeWhitespaces(parseContext *ctx)
{
    size_t *idx = ctx->index;

    for (; *idx < ctx->inputLength;)
    {
        char ch = ctx->input[*idx];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
        {
            return ERR_NOERROR;
        }

        (*idx)++;
    }

    return ERR_EOF;
}

struct parseStringContext
{
    parseContext *globalCtx;
    Buffer *buffer;
};

bool isDigit(char ch)
{
    return ch == '0' || ch == '1' || ch == '2'
        || ch == '3' || ch == '4' || ch == '5'
        || ch == '6' || ch == '7' || ch == '8'
        || ch == '9';
}

JSONError readUnicodeEscapedChar(parseStringContext *ctx)
{
    JSONError error;
    size_t *idx = ctx->globalCtx->index;

    if ((*idx) + 4 >= ctx->globalCtx->inputLength)
    {
        return ERR_EOF;
    }

    char buffer[7];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 2; i < 6; i++, (*idx)++)
    {
        buffer[i] = ctx->globalCtx->input[*idx];
    }
    buffer[6] = '\0';

    int32_t s = strtol(buffer, NULL, 16);

    char ch = (char) ((s & 0xFF00) >> 8);
    char ch2 = (char) (s & 0xFF);

    if ((error = putCharToBuffer(ctx->buffer, ch)) != ERR_NOERROR)
    {
        return error;
    }

    if ((error = putCharToBuffer(ctx->buffer, ch2)) != ERR_NOERROR)
    {
        return error;
    }

    return ERR_NOERROR;
}

JSONError parseString(parseStringContext *ctx)
{
    JSONError error;
    size_t *idx = ctx->globalCtx->index;

    // Prerequisites
    {
        if ((*idx + 2) >= ctx->globalCtx->inputLength)
        {
            return ERR_EOF;
        }

        char ch = ctx->globalCtx->input[(*idx)++];
        if (ch != '"')
        {
            return ERR_INVALID_STRING;
        }
    }

    bool sawBackslash = false;

    for (; *idx < ctx->globalCtx->inputLength; (*idx)++)
    {
        char ch = ctx->globalCtx->input[*idx];

        if (ch == '\\' && !sawBackslash)
        {
            sawBackslash = true;

            continue;
        }

        if (ch == '"')
        {
            if (!sawBackslash)
            {
                // end of string
                (*idx)++;
                goto done;
            }

            sawBackslash = false;

            if ((error = putCharToBuffer(ctx->buffer, ch)) != ERR_NOERROR)
            {
                return error;
            }

            continue;
        }

        if (sawBackslash && (ch == 'b' || ch == 'f' || ch == 'n' ||
                             ch == 'r' || ch == 't'))
        {
            switch (ch)
            {
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
            }

            if ((error = putCharToBuffer(ctx->buffer, ch)) != ERR_NOERROR)
            {
                return error;
            }

            sawBackslash = false;
            continue;
        }

        if (sawBackslash && ch == 'u')
        {
            (*idx)++; // eat the token

            if ((error = readUnicodeEscapedChar(ctx)) != ERR_NOERROR)
            {
                return error;
            }

            sawBackslash = false;
            continue;
        }

        if ((error = putCharToBuffer(ctx->buffer, ch)) != ERR_NOERROR)
        {
            return error;
        }
    }

done:

    if ((error = putCharToBuffer(ctx->buffer, '\0')) != ERR_NOERROR)
    {
        return error;
    }

    return ERR_NOERROR;
}

JSONError parseBoolean(parseContext *globalCtx, bool *ret)
{
    size_t *idx = globalCtx->index;
    char ch = globalCtx->input[*idx];
    size_t toRead = ch == 't' ? 3 : 4;

    if ((*idx + toRead) >= globalCtx->inputLength)
    {
        return ERR_EOF;
    }

    (*idx)++;

    // True case
    if (ch == 't')
    {
        if (globalCtx->input[(*idx)++] == 'r'
                && globalCtx->input[(*idx)++] == 'u'
                && globalCtx->input[(*idx)++] == 'e')
        {
            *ret = true;
            return ERR_NOERROR;
        }
    }

    // False case
    if (globalCtx->input[(*idx)++] == 'a'
            && globalCtx->input[(*idx)++] == 'l'
            && globalCtx->input[(*idx)++] == 's'
            && globalCtx->input[(*idx)++] == 'e')
    {
        *ret = false;
        return ERR_NOERROR;
    }

    return ERR_INVALID_BOOLEAN_SYNTAX;
}

JSONError parseNumber(parseContext *ctx, JSONNodeType *type, int8_t* bytes)
{
    size_t *idx = ctx->index;

    char buffer[1 << 16];
    bool isDouble = false;
    int i = 0;
    for (; *idx < ctx->inputLength; i++, (*idx)++)
    {
        char ch = ctx->input[*idx];
        if (ch == '.')
        {
            isDouble = true;
            buffer[i] = ch;
            continue;
        }

        if (!isDigit(ch) && ch != '-')
        {
            break;
        }

        buffer[i] = ch;
    }
    buffer[i] = '\0';

    if (isDouble)
    {
        *type = DOUBLE_NODE;
        *((double*) bytes) = atof(buffer);
    }
    else
    {
        *type = INTEGER_NODE;
        *((int64_t*) bytes) = _atoi64(buffer);
    }

    return ERR_NOERROR;
}

JSONError parseValue(JSONNode *value, parseContext *ctx, bool *parsedSomething)
{
    JSONError error = ERR_NOERROR;
    size_t *idx = ctx->index;

    char ch = ctx->input[*idx]; // do not eat the character

    if (ch == '{')
    {
        (*idx)++; // eat the token

        return parseObjectNode(value, ctx, parsedSomething);
    }

    if (ch == '[')
    {
        (*idx)++; // eat the token

        return parseArrayNode(value, ctx, parsedSomething);
    }

    if (ch == '"')
    {
        parseStringContext valCtx = {};
        valCtx.globalCtx = ctx;
        valCtx.buffer = newBuffer();

        if ((error = parseString(&valCtx)) != ERR_NOERROR)
        {
            return error;
        }

        value->type = STRING_NODE;
        value->stringValue = newJSONString();
        setJSONStringData(value->stringValue, valCtx.buffer);

        freeBuffer(valCtx.buffer);

        if (parsedSomething)
        {
            *parsedSomething = true;
        }

        return error;
    }

    if (ch == 't' || ch == 'f')
    {
        bool ret;
        if ((error = parseBoolean(ctx, &ret)) != ERR_NOERROR)
        {
            return error;
        }

        value->type = BOOLEAN_NODE;
        value->booleanValue = ret;

        if (parsedSomething)
        {
            *parsedSomething = true;
        }

        return error;
    }

    if (ch == 'n')
    {
        (*idx)++; // eat the token

        if ((*idx) + 3 >= ctx->inputLength)
        {
            return ERR_EOF;
        }

        char ch1 = ctx->input[(*idx)++];
        char ch2 = ctx->input[(*idx)++];
        char ch3 = ctx->input[(*idx)++];

        if (ch1 != 'u' && ch2 != 'l' && ch3 != 'l')
        {
            return ERR_INVALID_NULL_SYNTAX;
        }

        value->type = NULL_NODE;

        if (parsedSomething)
        {
            *parsedSomething = true;
        }

        return error;
    }

    if (isDigit(ch) || ch == '-')
    {
        int8_t bytes[8];

        if ((error = parseNumber(ctx, &value->type, (int8_t*) bytes)) != ERR_NOERROR)
        {
            return error;
        }

        if (value->type == DOUBLE_NODE)
        {
            value->doubleValue = *((double*) bytes);
        }
        else
        {
            value->intValue = *((int64_t*) bytes);
        }

        if (parsedSomething)
        {
            *parsedSomething = true;
        }
    }

    return error;
}

JSONError parseKeyValuePair(JSONString *key, JSONNode *value, parseContext *ctx, bool *parsedSomething)
{
    JSONError error;
    size_t *idx = ctx->index;

    {
        parseStringContext keyCtx = {};
        keyCtx.buffer = newBuffer();
        keyCtx.globalCtx = ctx;
        if ((error = parseString(&keyCtx)) != ERR_NOERROR)
        {
            return error;
        }

        setJSONStringData(key, keyCtx.buffer);

        freeBuffer(keyCtx.buffer);
    }

    // Prerequisites
    {
        if ((*idx + 1) >= ctx->inputLength)
        {
            return ERR_EOF;
        }

        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            return error;
        }

        char ch = ctx->input[(*idx)++];
        if (ch != ':')
        {
            return ERR_INVALID_OBJECT_SYNTAX;
        }

        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            return error;
        }
    }

    {
        if ((error = parseValue(value, ctx, parsedSomething)) != ERR_NOERROR)
        {
            return error;
        }
    }

    if (parsedSomething)
    {
        *parsedSomething = true;
    }

    return ERR_NOERROR;
}

JSONError parseObjectNode(JSONNode *node, parseContext *ctx, bool *parsedSomething)
{
    JSONError error = ERR_NOERROR;

    node->type = OBJECT_NODE;
    node->keys = newJSONStringArray(2048);
    node->values = (JSONNode*) malloc(sizeof(JSONNode) * MAX_VALUES); // TODO make this dynamic
	memset(node->values, 0, sizeof(JSONNode) * MAX_VALUES);
    size_t *idx = ctx->index;

    size_t keyValuePairIdx = 0;
    for (; *idx < ctx->inputLength;)
    {
        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            goto done;
        }

        // check early for end of object
        if (ctx->input[*idx] == '}')
        {
            (*idx)++;

            return consumeWhitespaces(ctx);
        }

        {
            JSONString *key = newJSONStringFromArray(node->keys);

            if ((error = parseKeyValuePair(key, &node->values[keyValuePairIdx], ctx, parsedSomething)) != ERR_NOERROR)
            {
                goto done;
            }

            if (parsedSomething && *parsedSomething)
            {
                node->length++;
            }
        }

        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            goto done;
        }

        {
            // parseKeyValuePair advances idx already
            char ch = ctx->input[*idx];

            // end of object
            if (ch == '}')
            {
                (*idx)++; // eat the token

                return consumeWhitespaces(ctx);
            }

            ch = ctx->input[*idx];
            if (ch == ',')
            {
                (*idx)++;

                keyValuePairIdx++;
            }
        }
    }

done:

    return error;
}

JSONError parseArrayNode(JSONNode *node, parseContext *ctx, bool *parsedSomething)
{
    JSONError error = ERR_NOERROR;

    node->type = ARRAY_NODE;
    node->values = (JSONNode*) malloc(sizeof(JSONNode) * MAX_VALUES); // TODO make this dynamic
    memset(node->values, 0, sizeof(JSONNode) * MAX_VALUES);

    size_t *idx = ctx->index;
    size_t elementIndex = 0;

    for (; *idx < ctx->inputLength;)
    {
        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            return error;
        }

        JSONNode *element = &node->values[elementIndex];

        if ((error = parseValue(element, ctx, parsedSomething)) != ERR_NOERROR)
        {
            return error;
        }

        if (parsedSomething && *parsedSomething)
        {
            node->length++;
        }

        if ((error = consumeWhitespaces(ctx)) != ERR_NOERROR)
        {
            return error;
        }

        char ch = ctx->input[*idx];
        if (ch == ',')
        {
            (*idx)++; // eat the token
             elementIndex++;
             continue;
        }

        if (ch == ']')
        {
            (*idx)++; // eat the token
            error = consumeWhitespaces(ctx);
            break;
        }

        return ERR_INVALID_ARRAY_SYNTAX;
    }

    return error;
}

JSON_API JSONNodeType JSONGetNodeType(JSONNode *node)
{
    if (!node)
    {
        return JSONNodeType::UNKNOWN;
    }

    return node->type;
}

JSON_API JSONString* JSONNodeGetString(JSONNode *node)
{
    if (!node || node->type != STRING_NODE)
    {
        return NULL;
    }

    return node->stringValue;
}

JSON_API bool JSONNodeGetBool(JSONNode *node)
{
    if (!node || node->type != BOOLEAN_NODE)
    {
        return false;
    }

    return node->booleanValue;
}

JSON_API int64_t JSONNodeGetInteger(JSONNode *node)
{
    if (!node || node->type != INTEGER_NODE)
    {
        return -1;
    }

    return node->intValue;
}

JSON_API double JSONNodeGetDouble(JSONNode *node)
{
    if (!node || node->type != DOUBLE_NODE)
    {
        return NAN;
    }

    return node->doubleValue;
}

JSON_API JSONNode* JSONCreateNode(void)
{
    JSONNode *node = (JSONNode *) malloc(sizeof(JSONNode));
    memset(node, 0, sizeof(JSONNode));

    return node;
}

JSON_API void JSONFreeNode(JSONNode *node)
{
    if (!node)
    {
        return;
    }

    if (node->keys)
    {
        freeJSONStringArray(node->keys);
    }

    for (size_t i = 0; i < node->length; i++)
    {
        JSONFreeNode(&node->values[i]);
    }

    // We only have to clean a string node, other node types are self-cleaning
    if (node->type == STRING_NODE)
    {
        cleanJSONString(node->stringValue);
    }
}

JSON_API JSONError JSONParse(JSONNode *tree, const char *input, size_t inputLength)
{
    JSONError error;
    size_t index = 0;

    parseContext ctx = {};
    ctx.input = input;
    ctx.inputLength = inputLength;

    ctx.index = &(index);
    switch (input[index++])
    {
        case '{':
        {
            bool parsedSomething = false;
            error = parseObjectNode(tree, &ctx, &parsedSomething);
            goto done;
        }
        case '[':
        {
            bool parsedSomething = false;
            error = parseArrayNode(tree, &ctx, &parsedSomething);
            goto done;
        }
        default:
        {
            return ERR_INVALID_TREE_SYNTAX;
        }
    }

done:
    if (error != ERR_NOERROR && error != ERR_EOF)
    {
        return error;
    }

    return ERR_NOERROR;
}

struct JSONIterator
{
    JSONNode *node;
    size_t index;
};

JSON_API JSONIterator* JSONCreateIterator(JSONNode *node)
{
    JSONIterator *iter = (JSONIterator*) malloc(sizeof(JSONIterator));
    iter->node = node;
    iter->index = 0;

    return iter;
}

JSON_API void JSONFreeIterator(JSONIterator *iter)
{
    free(iter);
}

JSON_API JSONError JSONIteratorGetNext(JSONIterator *iter, JSONString **key, JSONNode **value)
{
    if (!iter->node)
    {
        return ERR_ITERATOR_INVALID_NODE;
    }

    if (iter->node->type == OBJECT_NODE && !key)
    {
        return ERR_ITERATOR_INVALID_KEY_PTR;
    }

    if (!value)
    {
        return ERR_ITERATOR_INVALID_VALUE_PTR;
    }

    if (iter->index >= iter->node->length)
    {
        return ERR_ITERATOR_NO_MORE_ELEMENTS;
    }

    if (iter->node->type == OBJECT_NODE)
    {
        *key = &iter->node->keys->underlying[iter->index];
    }
    *value = &iter->node->values[iter->index];

    iter->index++;

    return ERR_NOERROR;
}
