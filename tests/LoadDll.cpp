#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>

#include "../MemoryModule.h"

typedef int (*addProc)(int);
typedef int (*addNumberProc)(int, int);

// Thanks to Tim Cooper (from http://stackoverflow.com/a/8584708)
const char *sstrstr(const char *haystack, const char *needle, size_t length) {
    size_t needle_length = strlen(needle);
    size_t i;

    for (i = 0; i < length; i++) {
        if (i + needle_length > length) {
            return NULL;
        }

        if (strncmp(&haystack[i], needle, needle_length) == 0) {
            return &haystack[i];
        }
    }
    return NULL;
}

const wchar_t *swcsstr(const wchar_t *haystack, const wchar_t *needle, size_t length) {
    size_t needle_length = wcslen(needle);
    size_t i;

    for (i = 0; i < length; i++) {
        if (i + needle_length > length) {
            return NULL;
        }

        if (wcsncmp(&haystack[i], needle, needle_length) == 0) {
            return &haystack[i];
        }
    }
    return NULL;
}

BOOL CheckResourceStrings(LPVOID data, DWORD size, const char *first, const wchar_t *second) {
    const char *first_pos;
    const wchar_t *second_pos;
    const wchar_t *src;

    if (data == NULL || size == 0) {
        return FALSE;
    }

    first_pos = sstrstr((const char *) data, first, size);
    if (first_pos == NULL) {
        fprintf(stderr, "ERROR: data doesn't start with %s\n", first);
        return FALSE;
    }

    src = (const wchar_t *) (((const char *) data) + strlen(first) + 1);
    second_pos = swcsstr(src, second, (size - strlen(first) - 1) / sizeof(wchar_t));
    if (second_pos == NULL) {
        fwprintf(stderr, L"ERROR: data doesn't continue with %s\n", second);
        return FALSE;
    }

    return TRUE;
}

BOOL LoadFromMemory(char *filename)
{
    FILE *fp;
    unsigned char *data=NULL;
    long size;
    size_t read;
    HMEMORYMODULE handle = NULL;
    addNumberProc addNumber;
    addNumberProc addNumber2;
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
    assert(size > 0);
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
        result = FALSE;
        goto exit;
    }

    addNumber = (addNumberProc)MemoryGetProcAddress(handle, NULL);
    if (addNumber != NULL) {
        _tprintf(_T("MemoryGetProcAddress(NULL) returned %p\n"), addNumber);
        result = FALSE;
        goto exit;
    }

    addNumber = (addNumberProc)MemoryGetProcAddress(handle, reinterpret_cast<LPCSTR>(0xff));
    if (addNumber != NULL) {
        _tprintf(_T("MemoryGetProcAddress(0xff) returned %p\n"), addNumber);
        result = FALSE;
        goto exit;
    }

    addNumber = (addNumberProc)MemoryGetProcAddress(handle, "addNumbers");
    if (!addNumber) {
        _tprintf(_T("MemoryGetProcAddress(\"addNumber\") returned NULL\n"));
        result = FALSE;
        goto exit;
    }
    _tprintf(_T("From memory: %d\n"), addNumber(1, 2));

    // the DLL only exports one function, try to load by ordinal value
    addNumber2 = (addNumberProc)MemoryGetProcAddress(handle, reinterpret_cast<LPCSTR>(0x01));
    if (addNumber != addNumber2) {
        _tprintf(_T("MemoryGetProcAddress(0x01) returned %p (expected %p)\n"), addNumber2, addNumber);
        result = FALSE;
        goto exit;
    }

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

    resourceInfo = MemoryFindResource(handle, _T("stringres"), RT_RCDATA);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);
    if (resourceInfo != NULL) {
        resourceSize = MemorySizeofResource(handle, resourceInfo);
        resourceData = MemoryLoadResource(handle, resourceInfo);

        _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);
        if (!CheckResourceStrings(resourceData, resourceSize, "This is a ANSI string", L"This is a UNICODE string")) {
            result = FALSE;
        }
    } else {
        result = FALSE;
    }

    resourceInfo = MemoryFindResource(handle, _T("stringres1"), RT_RCDATA);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);
    if (resourceInfo != NULL) {
        resourceSize = MemorySizeofResource(handle, resourceInfo);
        resourceData = MemoryLoadResource(handle, resourceInfo);

        _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);
        if (!CheckResourceStrings(resourceData, resourceSize, "This is ANSI string 1", L"This is UNICODE string 1")) {
            result = FALSE;
        }
    } else {
        result = FALSE;
    }


    resourceInfo = MemoryFindResource(handle, _T("stringres2"), RT_RCDATA);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);
    if (resourceInfo != NULL) {
        resourceSize = MemorySizeofResource(handle, resourceInfo);
        resourceData = MemoryLoadResource(handle, resourceInfo);

        _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);
        if (!CheckResourceStrings(resourceData, resourceSize, "This is ANSI string 2", L"This is UNICODE string 2")) {
            result = FALSE;
        }
    } else {
        result = FALSE;
    }


    resourceInfo = MemoryFindResource(handle, _T("stringres3"), RT_RCDATA);
    _tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);
    if (resourceInfo != NULL) {
        resourceSize = MemorySizeofResource(handle, resourceInfo);
        resourceData = MemoryLoadResource(handle, resourceInfo);

        _tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);
        if (!CheckResourceStrings(resourceData, resourceSize, "This is ANSI string 3", L"This is UNICODE string 3")) {
            result = FALSE;
        }
    } else {
        result = FALSE;
    }

exit:
    MemoryFreeLibrary(handle);
    free(data);
    return result;
}

BOOL LoadExportsFromMemory(char *filename)
{
    FILE *fp;
    unsigned char *data=NULL;
    long size;
    size_t read;
    HMEMORYMODULE handle = NULL;
    int i;
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
    assert(size > 0);
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
        result = FALSE;
        goto exit;
    }

    for (i = 1; i <= 100; i++) {
        char name[100];
        sprintf(name, "add%d", i);
        addProc addNumber = (addProc)MemoryGetProcAddress(handle, name);
        if (!addNumber) {
            _tprintf(_T("MemoryGetProcAddress(\"%s\") returned NULL\n"), name);
            result = FALSE;
            goto exit;
        }
        int result = addNumber(1);
        if (result != 1 + i) {
            _tprintf(_T("(\"%s\") returned %d, expected %d\n"), name, result, 1 + i);
            result = FALSE;
            goto exit;
        }
        _tprintf(_T("%s: %d\n"), name, result);
    }
exit:
    MemoryFreeLibrary(handle);
    free(data);
    return result;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s <filename.dll>\n", argv[0]);
        return 1;
    }

    if (!strstr((const char *) argv[1], "exports")) {
        if (!LoadFromMemory(argv[1])) {
            return 2;
        }
    } else {
        if (!LoadExportsFromMemory(argv[1])) {
            return 2;
        }
    }

    return 0;
}
