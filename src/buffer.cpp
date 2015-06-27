#include "buffer.h"

#include <stdlib.h>
#include <string.h>

Buffer *newBuffer()
{
    Buffer *buffer = (Buffer *) malloc(sizeof(Buffer));
    memset(buffer, 0, sizeof(Buffer));
    buffer->capacity = sizeof(char) * (1 << 16);
    buffer->underlying = (char *) malloc(buffer->capacity);

    return buffer;
}

void freeBuffer(Buffer *buffer)
{
    free(buffer->underlying);
    free(buffer);
}

void clearBuffer(Buffer *buffer)
{
    buffer->index = 0;
}

JSONError BufferGrow(Buffer *buffer)
{
    size_t newCapacity = (size_t) (sizeof(char) * buffer->capacity * 1.5f);
    char *newUnderlying = (char *) malloc(newCapacity);

    memcpy_s(newUnderlying, newCapacity, buffer->underlying, buffer->capacity);
    free(buffer->underlying);

    buffer->capacity = newCapacity;
    buffer->underlying = newUnderlying;

    return ERR_NOERROR;
}

JSONError putArrayToBuffer(Buffer *buffer, char *data, size_t dataLength)
{
    if ((buffer->index) + dataLength >= buffer->capacity)
    {
        JSONError error = BufferGrow(buffer);
        if (error != ERR_NOERROR)
        {
            return error;
        }
    }

    char *ptr = buffer->underlying + buffer->index;
    memcpy_s(ptr, buffer->capacity - buffer->index, data, dataLength);
    buffer->index += dataLength;

    return ERR_NOERROR;
}

JSONError copyBuffer(Buffer *dest, Buffer *source)
{
    if ((dest->index) + source->index >= dest->capacity)
    {
        JSONError error = BufferGrow(dest);
        if (error != ERR_NOERROR)
        {
            return error;
        }
    }

    return putArrayToBuffer(dest, source->underlying, source->index);
}

JSONError putCharToBuffer(Buffer *buffer, char ch)
{
    if ((buffer->index) + 1 >= buffer->capacity)
    {
        JSONError error = BufferGrow(buffer);
        if (error != ERR_NOERROR)
        {
            return error;
        }
    }

    buffer->underlying[buffer->index++] = ch;

    return ERR_NOERROR;
}

char* getDataFromBuffer(Buffer *buffer)
{
    return buffer->underlying;
}
