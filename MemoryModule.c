/*
 * Memory DLL loading code
 * Version 0.0.1
 *
 * Copyright (c) 2004 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// disable warnings about pointer <-> DWORD conversions
#pragma warning( disable : 4311 4312 )

#include <Windows.h>
#include <winnt.h>
#include "ntinternals.h"

#define DEBUG_OUTPUT 0

#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#include "MemoryModule.h"

typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#define GET_NT_HEADER(module) ((PIMAGE_NT_HEADERS)&((const unsigned char *)(module))[((PIMAGE_DOS_HEADER)(module))->e_lfanew])
#define GET_HEADER_DICTIONARY(module, idx)	&GET_NT_HEADER(module)->OptionalHeader.DataDirectory[idx]
#define CALCULATE_REAL_ADDRESS(base, offset) (((unsigned char *)(base)) + (offset))

// stores number of modules loaded
static DWORD ModuleCount = 0;

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
CopySections(const unsigned char *data, PIMAGE_NT_HEADERS old_headers, HMODULE module)
{
	DWORD i, size;
	LPVOID dest;
	PIMAGE_SECTION_HEADER section;
	
	section = IMAGE_FIRST_SECTION(GET_NT_HEADER(module));
	for (i=0; i<GET_NT_HEADER(module)->FileHeader.NumberOfSections; i++, section++)
	{
		if (section->SizeOfRawData == 0)
		{
			// section doesn't contain data in the dll itself, but may define
			// uninitialized data
			size = old_headers->OptionalHeader.SectionAlignment;
			if (size > 0)
			{
				dest = VirtualAlloc(CALCULATE_REAL_ADDRESS(module, section->VirtualAddress),
					size,
					MEM_COMMIT,
					PAGE_READWRITE);

				section->Misc.PhysicalAddress = (DWORD)dest;
				memset(dest, 0, size);
			}

			// section is empty
			continue;
		}

		// commit memory block and copy data from dll
		dest = VirtualAlloc(CALCULATE_REAL_ADDRESS(module, section->VirtualAddress),
							section->SizeOfRawData,
							MEM_COMMIT,
							PAGE_READWRITE);
		memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
		section->Misc.PhysicalAddress = (DWORD)dest;
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
FinalizeSections(HMODULE module)
{
	DWORD i;
	PIMAGE_SECTION_HEADER section;
	
	// loop through all sections and change access flags
	section = IMAGE_FIRST_SECTION(GET_NT_HEADER(module));
	for (i=0; i<GET_NT_HEADER(module)->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD protect, oldProtect, size;
		int executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
		int readable =   (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
		int writeable =  (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

		if (section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
		{
			// section is not needed any more and can safely be freed
			VirtualFree((LPVOID)section->Misc.PhysicalAddress, section->SizeOfRawData, MEM_DECOMMIT);
			continue;
		}

		// determine protection flags based on characteristics
		protect = ProtectionFlags[executable][readable][writeable];
		if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
			protect |= PAGE_NOCACHE;

		// determine size of region
		size = section->SizeOfRawData;
		if (size == 0)
		{
			if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
				size = GET_NT_HEADER(module)->OptionalHeader.SizeOfInitializedData;
			else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
				size = GET_NT_HEADER(module)->OptionalHeader.SizeOfUninitializedData;
		}

		if (size > 0)
		{
			// change memory access flags
			if (VirtualProtect((LPVOID)section->Misc.PhysicalAddress, section->SizeOfRawData, protect, &oldProtect) == 0)
#ifdef DEBUG_OUTPUT
				OutputLastError("Error protecting memory page")
#endif
			;
		}
	}
}

static void
PerformBaseRelocation(HMODULE module, DWORD delta)
{
	DWORD i;
	PIMAGE_DATA_DIRECTORY directory;
		
	directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
	if (directory->Size > 0)
	{
		PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)CALCULATE_REAL_ADDRESS(module, directory->VirtualAddress);
		for (; relocation->VirtualAddress > 0; )
		{
			unsigned char *dest = (unsigned char *)CALCULATE_REAL_ADDRESS(module, relocation->VirtualAddress);
			unsigned short *relInfo = (unsigned short *)CALCULATE_REAL_ADDRESS(relocation, IMAGE_SIZEOF_BASE_RELOCATION);
			for (i=0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++)
			{
				DWORD *patchAddrHL;
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
					patchAddrHL = (DWORD *)(dest + offset);
					*patchAddrHL += delta;
					break;

				default:
					//printf("Unknown relocation: %d\n", type);
					break;
				}
			}

			// advance to next relocation block
			(char *)relocation += relocation->SizeOfBlock;
		}
	}
}

static int
BuildImportTable(HMODULE module)
{
	int result=1;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (directory->Size > 0)
	{
		PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)CALCULATE_REAL_ADDRESS(module, directory->VirtualAddress);
		for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++)
		{
			DWORD *thunkRef, *funcRef;
			HMODULE handle = LoadLibrary((LPCSTR)CALCULATE_REAL_ADDRESS(module, importDesc->Name));
			if (handle == INVALID_HANDLE_VALUE)
			{
				SetLastError(ERROR_MOD_NOT_FOUND);
#if DEBUG_OUTPUT
				OutputLastError("Can't load library");
#endif
				result = 0;
				break;
			}

			if (importDesc->OriginalFirstThunk)
			{
				thunkRef = (DWORD *)CALCULATE_REAL_ADDRESS(module, importDesc->OriginalFirstThunk);
				funcRef = (DWORD *)CALCULATE_REAL_ADDRESS(module, importDesc->FirstThunk);
			} else {
				// no hint table
				thunkRef = (DWORD *)CALCULATE_REAL_ADDRESS(module, importDesc->FirstThunk);
				funcRef = (DWORD *)CALCULATE_REAL_ADDRESS(module, importDesc->FirstThunk);
			}
			for (; *thunkRef; thunkRef++, funcRef++)
			{
				if IMAGE_SNAP_BY_ORDINAL(*thunkRef)
					*funcRef = (DWORD)GetProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef));
				else {
					PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)CALCULATE_REAL_ADDRESS(module, *thunkRef);
					*funcRef = (DWORD)GetProcAddress(handle, (LPCSTR)&thunkData->Name);
				}
				if (*funcRef == 0)
				{
					SetLastError(ERROR_PROC_NOT_FOUND);
					result = 0;
					break;
				}
			}

			if (!result)
				break;
		}
	}

	return result;
}

static PPEB
GetPEB(void)
{
	// XXX: is there a better or even documented way to do this?
	__asm {
		// get PEB
		mov eax, dword ptr fs:[30h]
	}
}

static HMODULE
FindLibraryInPEB(const unsigned char *name, int incLoadCount)
{
	PPEB_LDR_DATA loaderData;
	PLDR_MODULE loaderModule;
	PWSTR longName;
	size_t i;
	HMODULE result=NULL;

	if (name == NULL)
		return NULL;

	// convert name to long character name
	longName = (PWSTR)calloc((strlen(name)+1)*2, 1);
	for (i=0; i<strlen(name);i++)
		longName[i] = name[i];

	// search module with given name in PEB
	loaderData = GetPEB()->LoaderData;
	loaderModule = (PLDR_MODULE)(loaderData->InLoadOrderModuleList.Flink);
	while (1)
	{
		if (wcsicmp(longName, loaderModule->BaseDllName.Buffer) == 0)
		{
			result = loaderModule->BaseAddress;
			if (incLoadCount && loaderModule->LoadCount != 0xffff)
				// we use this module, so increate the load count
				loaderModule->LoadCount++;
			
			goto exit;
		}

		// advance to next module
		loaderModule = (PLDR_MODULE)(loaderModule->InLoadOrderModuleList.Flink);
		if (loaderModule->BaseAddress == NULL || loaderModule == (PLDR_MODULE)(loaderData->InLoadOrderModuleList.Flink))
			// we traversed through the complete list
			// and didn't find the library
			goto exit;
	}

exit:
	free(longName);

	return result;
}

// Append a loader module to the end of the loader data list of the PEB
#define AppendToChain(module, list, chain) { \
	(module)->##chain##.Flink = (list)->##chain##.Flink; \
	(module)->##chain##.Blink = (list)->##chain##.Blink; \
	((PLDR_MODULE)((list)->##chain##.Blink))->##chain##.Flink = &(module)->##chain##; \
	(list)->##chain##.Blink = &(module)->##chain##; \
};

static PLDR_MODULE
InsertModuleInPEB(HMODULE module, unsigned char *name, unsigned char *baseName, DWORD locationDelta)
{
	PLDR_MODULE loaderModule;
	PPEB_LDR_DATA loaderData = GetPEB()->LoaderData;
	DWORD entry = GET_NT_HEADER(module)->OptionalHeader.AddressOfEntryPoint;
	size_t i;

	loaderModule = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LDR_MODULE));
	if (loaderModule == NULL)
		return NULL;

	loaderModule->BaseAddress = module;
	loaderModule->EntryPoint = (PVOID)(entry ? CALCULATE_REAL_ADDRESS(module, entry) : 0);
	loaderModule->SizeOfImage = GET_NT_HEADER(module)->OptionalHeader.SizeOfImage;
	loaderModule->LoadCount = 1;

	loaderModule->BaseDllName.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (strlen(baseName)+1)*2);
	if (loaderModule->BaseDllName.Buffer == NULL)
	{
		HeapFree(GetProcessHeap(), 0, loaderModule);
		return NULL;
	}
	loaderModule->BaseDllName.Length = (USHORT)strlen(baseName)*2;
	loaderModule->BaseDllName.MaximumLength = (USHORT)HeapSize(GetProcessHeap(), 0, loaderModule->BaseDllName.Buffer);
	for (i=0; i<strlen(baseName); i++)
		loaderModule->BaseDllName.Buffer[i] = baseName[i];

	loaderModule->FullDllName.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (strlen(name)+1)*2);
	if (loaderModule->BaseDllName.Buffer == NULL)
	{
		HeapFree(GetProcessHeap(), 0, loaderModule->BaseDllName.Buffer);
		HeapFree(GetProcessHeap(), 0, loaderModule);
		return NULL;
	}
	loaderModule->FullDllName.Length = (USHORT)strlen(name)*2;
	loaderModule->FullDllName.MaximumLength = (USHORT)HeapSize(GetProcessHeap(), 0, loaderModule->FullDllName.Buffer);
	for (i=0; i<strlen(name); i++)
		loaderModule->FullDllName.Buffer[i] = name[i];

	// XXX: are these the correct flags?
	loaderModule->Flags = IMAGE_DLL | ENTRY_PROCESSED;
	if (locationDelta != 0)
		loaderModule->Flags |= IMAGE_NOT_AT_BASE;
	loaderModule->TimeDateStamp = GET_NT_HEADER(module)->FileHeader.TimeDateStamp;

	// XXX: do we need more set the hash table?
	//loaderModule->HashTableEntry.Flink = &loaderModule->HashTableEntry;
	//loaderModule->HashTableEntry.Blink = &loaderModule->HashTableEntry;

	AppendToChain(loaderModule, loaderData, InLoadOrderModuleList);
	AppendToChain(loaderModule, loaderData, InInitializationOrderModuleList);

	// XXX: insert at the correct position in the chain
	AppendToChain(loaderModule, loaderData, InMemoryOrderModuleList);
	return loaderModule;
}

HMODULE MemoryLoadLibrary(const void *data, unsigned char *name)
{
	HMODULE result;
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS old_header;
	LPVOID headers;
	unsigned char *baseName;
	unsigned char fullname[MAX_DLL_NAME_LENGTH], tempname[MAX_DLL_NAME_LENGTH];
	DWORD locationDelta;
	DllEntryProc DllEntry;
	BOOL successfull;
	DWORD hasFullName;
	PLDR_MODULE loaderModule=NULL;
	
	// make sure we have a module name
	if (name == NULL || strlen(name) == 0)
	{
		sprintf(tempname, "memorymodule%d", ModuleCount);
		name = (unsigned char *)&tempname;
	}

	// maybe a module with the given name has been loaded already
	hasFullName = GetFullPathName(name, sizeof(fullname), (LPSTR)&fullname, &baseName);
	
	// search for module in PEB
	result = FindLibraryInPEB(hasFullName ? baseName : name, 1);
	if (result != NULL)
		// already loaded this module
		goto exit;
	
	if (hasFullName)
		// use complete filename as module name
		name = (unsigned char *)&fullname;
	else
		baseName = name;

	dos_header = (PIMAGE_DOS_HEADER)data;
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
	{
#if DEBUG_OUTPUT
		OutputDebugString("Not a valid executable file.\n");
#endif
		goto error;
	}

	old_header = GET_NT_HEADER(data);
	if (old_header->Signature != IMAGE_NT_SIGNATURE)
	{
#if DEBUG_OUTPUT
		OutputDebugString("No PE header found.\n");
#endif
		goto error;
	}

	// reserve memory for image of library
	result = (HMODULE)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
		old_header->OptionalHeader.SizeOfImage,
		MEM_RESERVE,
		PAGE_READWRITE);

    if (result == NULL)
        // try to allocate memory at arbitrary position
        result = (HMODULE)VirtualAlloc(NULL,
            old_header->OptionalHeader.SizeOfImage,
            MEM_RESERVE,
            PAGE_READWRITE);
    
	if (result == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		goto error;
	}

	// XXX: is it correct to commit the complete memory region at once?
    //      calling DllEntry raises an exception if we don't...
	VirtualAlloc(result,
		old_header->OptionalHeader.SizeOfImage,
		MEM_COMMIT,
		PAGE_READWRITE);

	// commit memory for headers
	headers = VirtualAlloc(result,
		dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders,
		MEM_COMMIT,
		PAGE_READWRITE);
	
	// copy PE header to code
	memcpy(headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders);
	
	// update position
	GET_NT_HEADER(result)->OptionalHeader.ImageBase = (DWORD)result;

	// copy sections from DLL file block to new memory location
	CopySections(data, old_header, result);

	// adjust base address of imported data
	locationDelta = (DWORD)((DWORD)result - old_header->OptionalHeader.ImageBase);
	if (locationDelta != 0)
		PerformBaseRelocation(result, locationDelta);

	// load required dlls and adjust function table of imports
	if (!BuildImportTable(result))
		goto error;

	// mark memory pages depending on section headers and release
	// sections that are marked as "discardable"
	FinalizeSections(result);

	// Add loaded module to PEB
	if (!(loaderModule = InsertModuleInPEB(result, name, baseName, locationDelta)))
		goto error;

	// get entry point of loaded library
	if (GET_NT_HEADER(result)->OptionalHeader.AddressOfEntryPoint != 0)
	{
		// notify library about attaching to process
		DllEntry = (DllEntryProc)CALCULATE_REAL_ADDRESS(result, GET_NT_HEADER(result)->OptionalHeader.AddressOfEntryPoint);
		successfull = (*DllEntry)(result, DLL_PROCESS_ATTACH, 0);
		if (!successfull)
		{
			SetLastError(ERROR_DLL_INIT_FAILED);
			goto error;
		}
		loaderModule->Flags |= PROCESS_ATTACH_CALLED;
	}

	ModuleCount++;
	goto exit;

error:
	// perform some cleanup...
	if (loaderModule != NULL)
	{
		if ((loaderModule->Flags & PROCESS_ATTACH_CALLED) != 0)
			(*DllEntry)(result, DLL_PROCESS_DETACH, 0);
		
		// remove from module chains
		loaderModule->InInitializationOrderModuleList.Flink->Blink = loaderModule->InInitializationOrderModuleList.Blink;
		loaderModule->InInitializationOrderModuleList.Blink->Flink = loaderModule->InInitializationOrderModuleList.Flink;
		loaderModule->InLoadOrderModuleList.Flink->Blink = loaderModule->InLoadOrderModuleList.Blink;
		loaderModule->InLoadOrderModuleList.Blink->Flink = loaderModule->InLoadOrderModuleList.Flink;
		loaderModule->InMemoryOrderModuleList.Flink->Blink = loaderModule->InMemoryOrderModuleList.Blink;
		loaderModule->InMemoryOrderModuleList.Blink->Flink = loaderModule->InMemoryOrderModuleList.Flink;

		// free memory for PEB structures
		HeapFree(GetProcessHeap(), 0, loaderModule->BaseDllName.Buffer);
		HeapFree(GetProcessHeap(), 0, loaderModule->FullDllName.Buffer);
		HeapFree(GetProcessHeap(), 0, loaderModule);
	}

	if (result != NULL)
		VirtualFree(result, 0, MEM_RELEASE);

	result = NULL;

exit:
	return result;
}
