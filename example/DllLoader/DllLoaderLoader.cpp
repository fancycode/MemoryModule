#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>

#include "../../MemoryModule.h"

#define EXE_FILE TEXT("DllLoader.exe")

int RunFromMemory(void)
{
    FILE *fp;
    unsigned char *data=NULL;
    long size;
    size_t read;
    HMEMORYMODULE handle;
    int result = -1;

    fp = _tfopen(EXE_FILE, _T("rb"));
    if (fp == NULL)
    {
        _tprintf(_T("Can't open executable \"%s\"."), EXE_FILE);
        goto exit;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    assert(size >= 0);
    data = (unsigned char *)malloc(size);
    assert(data != NULL);
    fseek(fp, 0, SEEK_SET);
    read = fread(data, 1, size, fp);
    assert(read == static_cast<size_t>(size));
    fclose(fp);

    handle = MemoryLoadLibrary(data, size);
    if (handle == NULL)
    {
        _tprintf(_T("Can't load library from memory.\n"));
        goto exit;
    }

    result = MemoryCallEntryPoint(handle);
    if (result < 0) {
        _tprintf(_T("Could not execute entry point: %d\n"), result);
    }
    MemoryFreeLibrary(handle);

exit:
    free(data);
    return result;
}

int main()
{
    return RunFromMemory();
}

