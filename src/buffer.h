#pragma once

#include "json.h"

struct Buffer
{
    char *underlying;
    size_t capacity;
    size_t index;
};

Buffer* newBuffer();
void freeBuffer(Buffer *buffer);
void clearBuffer(Buffer *buffer);
JSONError putArrayToBuffer(Buffer *buffer, char *data, size_t dataLength);
JSONError copyBuffer(Buffer *dest, Buffer *source);
JSONError putCharToBuffer(Buffer *buffer, char ch);
char* getDataFromBuffer(Buffer *buffer);
