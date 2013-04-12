/*
 * Memory DLL loading code
 * Version 0.0.3
 *
 * Copyright (c) 2004-2013 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is MemoryModule.c
 *
 * The Initial Developer of the Original Code is Joachim Bauch.
 *
 * Portions created by Joachim Bauch are Copyright (C) 2004-2013
 * Joachim Bauch. All Rights Reserved.
 *
 */

#ifndef __GNUC__
// disable warnings about pointer <-> DWORD conversions
#pragma warning( disable : 4311 4312 )
#endif

#ifdef _WIN64
#define POINTER_TYPE ULONGLONG
#else
#define POINTER_TYPE DWORD
#endif

#include <windows.h>
#include <winnt.h>
#include <tchar.h>
#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
// Vista SDKs no longer define IMAGE_SIZEOF_BASE_RELOCATION!?
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))
#endif

#include "MemoryModule.h"

typedef struct {
    PIMAGE_NT_HEADERS headers;
    unsigned char *codeBase;
    HCUSTOMMODULE *modules;
    int numModules;
    int initialized;
    CustomLoadLibraryFunc loadLibrary;
    CustomGetProcAddressFunc getProcAddress;
    CustomFreeLibraryFunc freeLibrary;
    void *userdata;
} MEMORYMODULE, *PMEMORYMODULE;

typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#define GET_HEADER_DICTIONARY(module, idx)	&(module)->headers->OptionalHeader.DataDirectory[idx]

#ifdef DEBUG_OUTPUT
static void
OutputLastError(const char *msg)
{
    LPVOID tmp;
    char *tmpmsg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
    tmpmsg = (char *)LocalAlloc(LPTR, strlen(msg) + strlen(tmp) + 3);
    sprintf(tmpmsg, "%s: %s", msg, tmp);
    OutputDebugString(tmpmsg);
    LocalFree(tmpmsg);
    LocalFree(tmp);
}
#endif

static void
CopySections(const unsigned char *data, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
{
    int i, size;
    unsigned char *codeBase = module->codeBase;
    unsigned char *dest;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
    for (i=0; i<module->headers->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData == 0) {
            // section doesn't contain data in the dll itself, but may define
            // uninitialized data
            size = old_headers->OptionalHeader.SectionAlignment;
            if (size > 0) {
                dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
                    size,
                    MEM_COMMIT,
                    PAGE_READWRITE);

                section->Misc.PhysicalAddress = (DWORD) (POINTER_TYPE) dest;
                memset(dest, 0, size);
            }

            // section is empty
            continue;
        }

        // commit memory block and copy data from dll
        dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
                            section->SizeOfRawData,
                            MEM_COMMIT,
                            PAGE_READWRITE);
        memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
        section->Misc.PhysicalAddress = (DWORD) (POINTER_TYPE) dest;
    }
}

// Protection flags for memory pages (Executable, Readable, Writeable)
static int ProtectionFlags[2][2][2] = {
    {
        // not executable
        {PAGE_NOACCESS, PAGE_WRITECOPY},
        {PAGE_READONLY, PAGE_READWRITE},
    }, {
        // executable
        {PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
        {PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
    },
};

static void
FinalizeSections(PMEMORYMODULE module)
{
    int i;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
#ifdef _WIN64
    POINTER_TYPE imageOffset = (module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
    #define imageOffset 0
#endif
    
    // loop through all sections and change access flags
    for (i=0; i<module->headers->FileHeader.NumberOfSections; i++, section++) {
        DWORD protect, oldProtect, size;
        int executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        int readable =   (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
        int writeable =  (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

        if (section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
            // section is not needed any more and can safely be freed
            VirtualFree((LPVOID)((POINTER_TYPE)section->Misc.PhysicalAddress | imageOffset), section->SizeOfRawData, MEM_DECOMMIT);
            continue;
        }

        // determine protection flags based on characteristics
        protect = ProtectionFlags[executable][readable][writeable];
        if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) {
            protect |= PAGE_NOCACHE;
        }

        // determine size of region
        size = section->SizeOfRawData;
        if (size == 0) {
            if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
                size = module->headers->OptionalHeader.SizeOfInitializedData;
            } else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
                size = module->headers->OptionalHeader.SizeOfUninitializedData;
            }
        }

        if (size > 0) {
            // change memory access flags
            if (VirtualProtect((LPVOID)((POINTER_TYPE)section->Misc.PhysicalAddress | imageOffset), size, protect, &oldProtect) == 0)
#ifdef DEBUG_OUTPUT
                OutputLastError("Error protecting memory page")
#endif
            ;
        }
    }
#ifndef _WIN64
#undef imageOffset
#endif
}

static void
PerformBaseRelocation(PMEMORYMODULE module, SIZE_T delta)
{
    DWORD i;
    unsigned char *codeBase = module->codeBase;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
    if (directory->Size > 0) {
        PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);
        for (; relocation->VirtualAddress > 0; ) {
            unsigned char *dest = codeBase + relocation->VirtualAddress;
            unsigned short *relInfo = (unsigned short *)((unsigned char *)relocation + IMAGE_SIZEOF_BASE_RELOCATION);
            for (i=0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++) {
                DWORD *patchAddrHL;
#ifdef _WIN64
                ULONGLONG *patchAddr64;
#endif
                int type, offset;

                // the upper 4 bits define the type of relocation
                type = *relInfo >> 12;
                // the lower 12 bits define the offset
                offset = *relInfo & 0xfff;
                
                switch (type)
                {
                case IMAGE_REL_BASED_ABSOLUTE:
                    // skip relocation
                    break;

                case IMAGE_REL_BASED_HIGHLOW:
                    // change complete 32 bit address
                    patchAddrHL = (DWORD *) (dest + offset);
                    *patchAddrHL += (DWORD) delta;
                    break;
                
#ifdef _WIN64
                case IMAGE_REL_BASED_DIR64:
                    patchAddr64 = (ULONGLONG *) (dest + offset);
                    *patchAddr64 += (ULONGLONG) delta;
                    break;
#endif

                default:
                    //printf("Unknown relocation: %d\n", type);
                    break;
                }
            }

            // advance to next relocation block
            relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
        }
    }
}

static int
BuildImportTable(PMEMORYMODULE module)
{
    int result=1;
    unsigned char *codeBase = module->codeBase;
    HCUSTOMMODULE *tmp;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
    if (directory->Size > 0) {
        PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);
        for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++) {
            POINTER_TYPE *thunkRef;
            FARPROC *funcRef;
            HCUSTOMMODULE handle = module->loadLibrary((LPCSTR) (codeBase + importDesc->Name), module->userdata);
            if (handle == NULL) {
                SetLastError(ERROR_MOD_NOT_FOUND);
                result = 0;
                break;
            }

            tmp = (HCUSTOMMODULE *) realloc(module->modules, (module->numModules+1)*(sizeof(HCUSTOMMODULE)));
            if (tmp == NULL) {
                module->freeLibrary(handle, module->userdata);
                SetLastError(ERROR_OUTOFMEMORY);
                result = 0;
                break;
            }
            module->modules = tmp;

            module->modules[module->numModules++] = handle;
            if (importDesc->OriginalFirstThunk) {
                thunkRef = (POINTER_TYPE *) (codeBase + importDesc->OriginalFirstThunk);
                funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
            } else {
                // no hint table
                thunkRef = (POINTER_TYPE *) (codeBase + importDesc->FirstThunk);
                funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
            }
            for (; *thunkRef; thunkRef++, funcRef++) {
                if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                    *funcRef = module->getProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef), module->userdata);
                } else {
                    PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
                    *funcRef = module->getProcAddress(handle, (LPCSTR)&thunkData->Name, module->userdata);
                }
                if (*funcRef == 0) {
                    result = 0;
                    break;
                }
            }

            if (!result) {
                module->freeLibrary(handle, module->userdata);
                SetLastError(ERROR_PROC_NOT_FOUND);
                break;
            }
        }
    }

    return result;
}

static HCUSTOMMODULE _LoadLibrary(LPCSTR filename, void *userdata)
{
    HMODULE result = LoadLibraryA(filename);
    if (result == NULL) {
        return NULL;
    }
    
    return (HCUSTOMMODULE) result;
}

static FARPROC _GetProcAddress(HCUSTOMMODULE module, LPCSTR name, void *userdata)
{
    return (FARPROC) GetProcAddress((HMODULE) module, name);
}

static void _FreeLibrary(HCUSTOMMODULE module, void *userdata)
{
    FreeLibrary((HMODULE) module);
}

HMEMORYMODULE MemoryLoadLibrary(const void *data)
{
    return MemoryLoadLibraryEx(data, _LoadLibrary, _GetProcAddress, _FreeLibrary, NULL);
}

HMEMORYMODULE MemoryLoadLibraryEx(const void *data,
    CustomLoadLibraryFunc loadLibrary,
    CustomGetProcAddressFunc getProcAddress,
    CustomFreeLibraryFunc freeLibrary,
    void *userdata)
{
    PMEMORYMODULE result;
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS old_header;
    unsigned char *code, *headers;
    SIZE_T locationDelta;
    DllEntryProc DllEntry;
    BOOL successfull;

    dos_header = (PIMAGE_DOS_HEADER)data;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *)(data))[dos_header->e_lfanew];
    if (old_header->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // reserve memory for image of library
    // XXX: is it correct to commit the complete memory region at once?
    //      calling DllEntry raises an exception if we don't...
    code = (unsigned char *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
        old_header->OptionalHeader.SizeOfImage,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    if (code == NULL) {
        // try to allocate memory at arbitrary position
        code = (unsigned char *)VirtualAlloc(NULL,
            old_header->OptionalHeader.SizeOfImage,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);
        if (code == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }
    
    result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), 0, sizeof(MEMORYMODULE));
    if (result == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        VirtualFree(code, 0, MEM_RELEASE);
        return NULL;
    }

    result->codeBase = code;
    result->numModules = 0;
    result->modules = NULL;
    result->initialized = 0;
    result->loadLibrary = loadLibrary;
    result->getProcAddress = getProcAddress;
    result->freeLibrary = freeLibrary;
    result->userdata = userdata;

    // commit memory for headers
    headers = (unsigned char *)VirtualAlloc(code,
        old_header->OptionalHeader.SizeOfHeaders,
        MEM_COMMIT,
        PAGE_READWRITE);
    
    // copy PE header to code
    memcpy(headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders);
    result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

    // update position
    result->headers->OptionalHeader.ImageBase = (POINTER_TYPE)code;

    // copy sections from DLL file block to new memory location
    CopySections(data, old_header, result);

    // adjust base address of imported data
    locationDelta = (SIZE_T)(code - old_header->OptionalHeader.ImageBase);
    if (locationDelta != 0) {
        PerformBaseRelocation(result, locationDelta);
    }

    // load required dlls and adjust function table of imports
    if (!BuildImportTable(result)) {
        goto error;
    }

    // mark memory pages depending on section headers and release
    // sections that are marked as "discardable"
    FinalizeSections(result);

    // get entry point of loaded library
    if (result->headers->OptionalHeader.AddressOfEntryPoint != 0) {
        DllEntry = (DllEntryProc) (code + result->headers->OptionalHeader.AddressOfEntryPoint);
        // notify library about attaching to process
        successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0);
        if (!successfull) {
            SetLastError(ERROR_DLL_INIT_FAILED);
            goto error;
        }
        result->initialized = 1;
    }

    return (HMEMORYMODULE)result;

error:
    // cleanup
    MemoryFreeLibrary(result);
    return NULL;
}

FARPROC MemoryGetProcAddress(HMEMORYMODULE module, LPCSTR name)
{
    unsigned char *codeBase = ((PMEMORYMODULE)module)->codeBase;
    int idx=-1;
    DWORD i, *nameRef;
    WORD *ordinal;
    PIMAGE_EXPORT_DIRECTORY exports;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE)module, IMAGE_DIRECTORY_ENTRY_EXPORT);
    if (directory->Size == 0) {
        // no export table found
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    exports = (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0) {
        // DLL doesn't export anything
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // search function name in list of exported names
    nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
    ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);
    for (i=0; i<exports->NumberOfNames; i++, nameRef++, ordinal++) {
        if (_stricmp(name, (const char *) (codeBase + (*nameRef))) == 0) {
            idx = *ordinal;
            break;
        }
    }

    if (idx == -1) {
        // exported symbol not found
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    if ((DWORD)idx > exports->NumberOfFunctions) {
        // name <-> ordinal number don't match
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // AddressOfFunctions contains the RVAs to the "real" functions
    return (FARPROC) (codeBase + (*(DWORD *) (codeBase + exports->AddressOfFunctions + (idx*4))));
}

void MemoryFreeLibrary(HMEMORYMODULE mod)
{
    int i;
    PMEMORYMODULE module = (PMEMORYMODULE)mod;

    if (module != NULL) {
        if (module->initialized != 0) {
            // notify library about detaching from process
            DllEntryProc DllEntry = (DllEntryProc) (module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
            (*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
            module->initialized = 0;
        }

        if (module->modules != NULL) {
            // free previously opened libraries
            for (i=0; i<module->numModules; i++) {
                if (module->modules[i] != NULL) {
                    module->freeLibrary(module->modules[i], module->userdata);
                }
            }

            free(module->modules);
        }

        if (module->codeBase != NULL) {
            // release memory of library
            VirtualFree(module->codeBase, 0, MEM_RELEASE);
        }

        HeapFree(GetProcessHeap(), 0, module);
    }
}

#define DEFAULT_LANGUAGE        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

HMEMORYRSRC MemoryFindResource(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type)
{
    return MemoryFindResourceEx(module, name, type, DEFAULT_LANGUAGE);
}

static PIMAGE_RESOURCE_DIRECTORY_ENTRY _MemorySearchResourceEntry(
    void *root,
    PIMAGE_RESOURCE_DIRECTORY resources,
    LPCTSTR key)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entries = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (resources + 1);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY result = NULL;
    DWORD start;
    DWORD end;
    DWORD middle;
    
    if (!IS_INTRESOURCE(key) && key[0] == TEXT('#')) {
        // special case: resource id given as string
        TCHAR *endpos = NULL;
#if defined(UNICODE)
        long int tmpkey = (WORD) wcstol((TCHAR *) &key[1], &endpos, 10);
#else
        long int tmpkey = (WORD) strtol((TCHAR *) &key[1], &endpos, 10);
#endif
        if (tmpkey <= 0xffff && lstrlen(endpos) == 0) {
            key = MAKEINTRESOURCE(tmpkey);
        }
    }
    
    // entries are stored as ordered list of named entries,
    // followed by an ordered list of id entries - we can do
    // a binary search to find faster...
    if (IS_INTRESOURCE(key)) {
        WORD check = (WORD) (POINTER_TYPE) key;
        start = resources->NumberOfNamedEntries;
        end = start + resources->NumberOfIdEntries;
        
        while (end > start) {
            WORD entryName;
            middle = (start + end) >> 1;
            entryName = (WORD) entries[middle].Name;
            if (check < entryName) {
                end = (end != middle ? middle : middle-1);
            } else if (check > entryName) {
                start = (start != middle ? middle : middle+1);
            } else {
                result = &entries[middle];
                break;
            }
        }
    } else {
#if !defined(UNICODE)
        char *searchKey = NULL;
        int searchKeyLength = 0;
#endif
        start = 0;
        end = resources->NumberOfIdEntries;
        while (end > start) {
            // resource names are always stored using 16bit characters
            int cmp;
			PIMAGE_RESOURCE_DIR_STRING_U resourceString;
            middle = (start + end) >> 1;
            resourceString = (PIMAGE_RESOURCE_DIR_STRING_U) (((char *) root) + (entries[middle].Name & 0x7FFFFFFF));
#if !defined(UNICODE)
            if (searchKey == NULL || searchKeyLength < resourceString->Length) {
                void *tmp = realloc(searchKey, resourceString->Length);
                if (tmp == NULL) {
                    break;
                }
                
                searchKey = (char *) tmp;
            }
            wcstombs(searchKey, resourceString->NameString, resourceString->Length);
            cmp = strncmp(key, searchKey, resourceString->Length);
#else
            cmp = wcsncmp(key, resourceString->NameString, resourceString->Length);
#endif
            if (cmp < 0) {
                end = (middle != end ? middle : middle-1);
            } else if (cmp > 0) {
                start = (middle != start ? middle : middle+1);
            } else {
                result = &entries[middle];
                break;
            }
        }
#if !defined(UNICODE)
        free(searchKey);
#endif
    }
    
    
    return result;
}

HMEMORYRSRC MemoryFindResourceEx(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type, WORD language)
{
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE) module, IMAGE_DIRECTORY_ENTRY_RESOURCE);
    PIMAGE_RESOURCE_DIRECTORY rootResources;
    PIMAGE_RESOURCE_DIRECTORY nameResources;
    PIMAGE_RESOURCE_DIRECTORY typeResources;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundLanguage;
    if (directory->Size == 0) {
        // no resource table found
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return NULL;
    }
    
    if (language == DEFAULT_LANGUAGE) {
        // use language from current thread
        language = LANGIDFROMLCID(GetThreadLocale());
    }

    // resources are stored as three-level tree
    // - first node is the type
    // - second node is the name
    // - third node is the language
    rootResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress);
    foundType = _MemorySearchResourceEntry(rootResources, rootResources, type);
    if (foundType == NULL) {
        SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
        return NULL;
    }
    
    typeResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress + (foundType->OffsetToData & 0x7fffffff));
    foundName = _MemorySearchResourceEntry(rootResources, typeResources, name);
    if (foundName == NULL) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }
    
    nameResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress + (foundName->OffsetToData & 0x7fffffff));
    foundLanguage = _MemorySearchResourceEntry(rootResources, nameResources, (LPCTSTR) (POINTER_TYPE) language);
    if (foundLanguage == NULL) {
        // requested language not found, use first available
        if (nameResources->NumberOfIdEntries == 0) {
            SetLastError(ERROR_RESOURCE_LANG_NOT_FOUND);
            return NULL;
        }
        
        foundLanguage = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (nameResources + 1);
    }
    
    return (codeBase + directory->VirtualAddress + (foundLanguage->OffsetToData & 0x7fffffff));
}

DWORD MemorySizeofResource(HMEMORYMODULE module, HMEMORYRSRC resource)
{
    PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    
    return entry->Size;
}

LPVOID MemoryLoadResource(HMEMORYMODULE module, HMEMORYRSRC resource)
{
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    
    return codeBase + entry->OffsetToData;
}

int
MemoryLoadString(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize)
{
    return MemoryLoadStringEx(module, id, buffer, maxsize, DEFAULT_LANGUAGE);
}

int
MemoryLoadStringEx(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize, WORD language)
{
	HMEMORYRSRC resource;
	PIMAGE_RESOURCE_DIR_STRING_U data;
	DWORD size;
    if (maxsize == 0) {
        return 0;
    }
    
    resource = MemoryFindResourceEx(module, MAKEINTRESOURCE((id >> 4) + 1), RT_STRING, language);
    if (resource == NULL) {
        buffer[0] = 0;
        return 0;
    }
    
    data = MemoryLoadResource(module, resource);
    id = id & 0x0f;
    while (id--) {
        data = (PIMAGE_RESOURCE_DIR_STRING_U) (((char *) data) + (data->Length + 1) * sizeof(WCHAR));
    }
    if (data->Length == 0) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        buffer[0] = 0;
        return 0;
    }
    
    size = data->Length;
    if (size >= (DWORD) maxsize) {
        size = maxsize;
    } else {
        buffer[size] = 0;
    }
#if defined(UNICODE)
    wcsncpy(buffer, data->NameString, size);
#else
    wcstombs(buffer, data->NameString, size);
#endif
    return size;
}
