#include "json.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

void putIndent(char *buffer, int size)
{
    for (int i = 0; i < size; i++)
    {
        buffer[i] = ' ';
    }
}

void printValue(JSONNode *node, const int indent)
{
    JSONError error;
    JSONNodeType type = JSONGetNodeType(node);

    char buffer[2048];
    memset(buffer, 0, 2048);

    putIndent(buffer, indent);
    char *ptr = (char*) buffer;

    switch (type)
    {
        case ARRAY_NODE:
        {
            OutputDebugStringA(JSONNodeTypeToString(type));
            OutputDebugStringA("\n");

            JSONIterator *newIter = JSONCreateIterator(node);
            JSONNode *value;

            while ((error = JSONIteratorGetNext(newIter, NULL, &value)) != ERR_ITERATOR_NO_MORE_ELEMENTS)
            {
                printValue(value, indent + 1);
            }

            break;

        }
        case OBJECT_NODE:
        {
            JSONIterator *iter = JSONCreateIterator(node);
            JSONString *key;
            JSONNode *value;

            OutputDebugStringA(JSONNodeTypeToString(type));
            OutputDebugStringA("\n");

            while ((error = JSONIteratorGetNext(iter, &key, &value)) != ERR_ITERATOR_NO_MORE_ELEMENTS)
            {
                // Print the key
                sprintf_s(ptr + indent, 2048, "key: %-5s\n", key->data);
                OutputDebugStringA(ptr);

                printValue(value, indent + 1);
            }

            break;
        }
        case STRING_NODE:
        {
            JSONString *string = JSONNodeGetString(node);

            sprintf_s(ptr + indent, 2048, "%s => %s\n", JSONNodeTypeToString(type), string->data);
            OutputDebugStringA(buffer);

            break;
        }
        case BOOLEAN_NODE:
        {
            bool val = JSONNodeGetBool(node);

            sprintf_s(ptr + indent, 2048, "%s => %s\n", JSONNodeTypeToString(type), val ? "true" : "false");
            OutputDebugStringA(buffer);

            break;
        }
        case INTEGER_NODE:
        {
            int64_t val = JSONNodeGetInteger(node);

            sprintf_s(ptr + indent, 2048, "%s => %d\n", JSONNodeTypeToString(type), val);
            OutputDebugStringA(buffer);

            break;
        }
        case DOUBLE_NODE:
        {
            double val = JSONNodeGetDouble(node);

            sprintf_s(ptr + indent, 2048, "%s => %0.2f\n", JSONNodeTypeToString(type), val);
            OutputDebugStringA(buffer);

            break;
        }
        case NULL_NODE:
        {
            sprintf_s(ptr + indent, 2048, "%s\n", JSONNodeTypeToString(type));
            OutputDebugStringA(buffer);

            break;
        }
    }
}

char* readFile()
{
    char *buffer = (char*) malloc(sizeof(char) * 1024 * 100);
    OFSTRUCT ofStruct;

    HFILE file = OpenFile("N:/my.json", &ofStruct, OF_READ);
    if (file == HFILE_ERROR)
    {
        free(buffer);
        OutputDebugStringA("Error while opening file");
        return NULL;
    }

    size_t totalRead = 0;
    DWORD bytesRead;
    BOOL readResult;

    for (;;)
    {
        readResult = ReadFile((HANDLE) file, (LPVOID) buffer, 4096, &bytesRead, NULL);
        if (readResult && bytesRead == 0)
        {
            break;
        }

        totalRead += bytesRead;
    }

    buffer[totalRead] = '\0';

    CloseHandle((HANDLE) file);

    return buffer;
}

int main(void)
{
    char *input = readFile();
    if (input == NULL)
    {
        return 1;
    }

    JSONError error;
    JSONNode *tree = JSONCreateTree();

    error = JSONParseTree(tree, (char*) input, strlen(input));
    if (error != ERR_NOERROR)
    {
        OutputDebugStringA("Error while parsing tree !!!\n");
        return 1;
    }

    printValue(tree, 0);

    return 0;
}
