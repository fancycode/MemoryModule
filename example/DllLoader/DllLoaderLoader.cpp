#define WIN32_LEAN_AND_MEAN

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
    size_t size;
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
    data = (unsigned char *)malloc(size);
    fseek(fp, 0, SEEK_SET);
    fread(data, 1, size, fp);
    fclose(fp);

    handle = MemoryLoadLibrary(data);
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
    if (data)
        free(data);
    return result;
}

int main(int argc, char* argv[])
{
    return RunFromMemory();
}

