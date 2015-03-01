#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>

#include "../MemoryModule.h"

typedef int (*addNumberProc)(int, int);

BOOL LoadFromMemory(char *filename)
{
    FILE *fp;
    unsigned char *data=NULL;
    size_t size;
    HMEMORYMODULE handle;
    addNumberProc addNumber;
    HMEMORYRSRC resourceInfo;
    DWORD resourceSize;
    LPVOID resourceData;
    TCHAR buffer[100];
    BOOL result = TRUE;

    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Can't open DLL file \"%s\".", filename);
        result = FALSE;
        goto exit;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    data = (unsigned char *)malloc(size);
    fseek(fp, 0, SEEK_SET);
    fread(data, 1, size, fp);
    fclose(fp);

    handle = MemoryLoadLibrary(data);
    if (handle == NULL)
    {
        _tprintf(_T("Can't load library from memory.\n"));
        result = FALSE;
        goto exit;
    }

    addNumber = (addNumberProc)MemoryGetProcAddress(handle, "addNumbers");
    _tprintf(_T("From memory: %d\n"), addNumber(1, 2));

    resourceInfo = MemoryFindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);

    if (resourceInfo != NULL) {
        resourceSize = MemorySizeofResource(handle, resourceInfo);
        resourceData = MemoryLoadResource(handle, resourceInfo);
        _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

        MemoryLoadString(handle, 1, buffer, sizeof(buffer));
        _tprintf(_T("String1: %s\n"), buffer);

        MemoryLoadString(handle, 20, buffer, sizeof(buffer));
        _tprintf(_T("String2: %s\n"), buffer);
    } else {
        result = FALSE;
    }

    MemoryFreeLibrary(handle);

exit:
    if (data)
        free(data);
    return result;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s <filename.dll>\n", argv[0]);
        return 1;
    }

    if (!LoadFromMemory(argv[1])) {
        return 2;
    }

    return 0;
}
