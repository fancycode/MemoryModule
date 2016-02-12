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

typedef int (*addNumberProc)(int, int);

#define DLL_FILE TEXT("..\\SampleDLL\\SampleDLL.dll")

void LoadFromFile(void)
{
    addNumberProc addNumber;
    HRSRC resourceInfo;
    DWORD resourceSize;
    LPVOID resourceData;
    TCHAR buffer[100];

    HINSTANCE handle = LoadLibrary(DLL_FILE);
    if (handle == NULL)
        return;

    addNumber = (addNumberProc)GetProcAddress(handle, "addNumbers");
    _tprintf(_T("From file: %d\n"), addNumber(1, 2));

    resourceInfo = FindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    _tprintf(_T("FindResource returned 0x%p\n"), resourceInfo);

    resourceSize = SizeofResource(handle, resourceInfo);
    resourceData = LoadResource(handle, resourceInfo);
    _tprintf(_T("Resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

    LoadString(handle, 1, buffer, sizeof(buffer));
    _tprintf(_T("String1: %s\n"), buffer);

    LoadString(handle, 20, buffer, sizeof(buffer));
    _tprintf(_T("String2: %s\n"), buffer);

    FreeLibrary(handle);
}

void* ReadLibrary(long* pSize) {
    long read;
    void* result;
    FILE* fp;

    fp = _tfopen(DLL_FILE, _T("rb"));
    if (fp == NULL)
    {
        _tprintf(_T("Can't open DLL file \"%s\"."), DLL_FILE);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *pSize = ftell(fp);
    if (*pSize < 0)
    {
        fclose(fp);
        return NULL;
    }

    result = (unsigned char *)malloc(*pSize);
    if (result == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    read = fread(result, 1, *pSize, fp);
    fclose(fp);
    if (read != static_cast<size_t>(*pSize))
    {
        free(result);
        return NULL;
    }

    return result;
}

void LoadFromMemory(void)
{
    void *data;
    long size;
    HMEMORYMODULE handle;
    addNumberProc addNumber;
    HMEMORYRSRC resourceInfo;
    DWORD resourceSize;
    LPVOID resourceData;
    TCHAR buffer[100];

    data = ReadLibrary(&size);
    if (data == NULL)
    {
        return;
    }

    handle = MemoryLoadLibrary(data, size);
    if (handle == NULL)
    {
        _tprintf(_T("Can't load library from memory.\n"));
        goto exit;
    }

    addNumber = (addNumberProc)MemoryGetProcAddress(handle, "addNumbers");
    _tprintf(_T("From memory: %d\n"), addNumber(1, 2));

    resourceInfo = MemoryFindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);

    resourceSize = MemorySizeofResource(handle, resourceInfo);
    resourceData = MemoryLoadResource(handle, resourceInfo);
    _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

    MemoryLoadString(handle, 1, buffer, sizeof(buffer));
    _tprintf(_T("String1: %s\n"), buffer);

    MemoryLoadString(handle, 20, buffer, sizeof(buffer));
    _tprintf(_T("String2: %s\n"), buffer);

    MemoryFreeLibrary(handle);

exit:
    free(data);
}

int main()
{
    LoadFromFile();
    printf("\n\n");
    LoadFromMemory();
    return 0;
}

