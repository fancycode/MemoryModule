#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "../../ntinternals.h"
#include "../../MemoryModule.h"

typedef int (*addNumberProc)(int, int);

#define DLL_FILE "..\\..\\SampleDLL\\Debug\\SampleDLL.dll"

PPEB GetPEB(void)
{
	// XXX: is there a better or even documented way to do this?
	__asm {
		// get PEB
		mov eax, dword ptr fs:[30h]
	}
}

void DumpPEB(void)
{
	PPEB peb = GetPEB();
	PPEB_LDR_DATA loaderData = peb->LoaderData;
	PLDR_MODULE loaderModule;

	printf("-------------------------------------\n");
	printf("PEB at 0x%x\n", (DWORD)peb);
	printf("Modules (Load Order)\n");
	loaderModule = (PLDR_MODULE)(loaderData->InLoadOrderModuleList.Flink);
	printf("Last: %x\n", (DWORD)loaderData->InLoadOrderModuleList.Blink);
	while (1)
	{
		printf("Info: %x\n", (DWORD)loaderModule);
		if (!IsBadReadPtr(loaderModule->BaseDllName.Buffer, loaderModule->BaseDllName.Length))
		{
			wprintf(L"Library: %s\n", loaderModule->BaseDllName.Buffer);
			wprintf(L"Fullname: %s\n", loaderModule->FullDllName.Buffer);
		} else
			printf("Unknown module\n");
		printf("Address: %x\n", (DWORD)loaderModule->BaseAddress);
		printf("Load count: %d\n", loaderModule->LoadCount);
		printf("Flags: %d\n", loaderModule->Flags);
		printf("Size: %d\n", loaderModule->SizeOfImage);
		printf("Entry: %x\n", (DWORD)loaderModule->EntryPoint);
		printf("Hash: %x %x\n", (DWORD)loaderModule->HashTableEntry.Flink, (DWORD)loaderModule->HashTableEntry.Blink);

		// advance to next module
		loaderModule = (PLDR_MODULE)(loaderModule->InLoadOrderModuleList.Flink);
		if (loaderModule == (PLDR_MODULE)(loaderData->InLoadOrderModuleList.Flink))
			// we traversed through the complete list
			// and didn't find the library
			goto exit;

		printf("\n");
	}

exit:
	printf("=====================================\n");
	printf("\n");
}

static void
OutputLastError(const char *msg)
{
	LPVOID tmp;
	char *tmpmsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
	tmpmsg = (char *)malloc(strlen(msg) + strlen((const char *)tmp) + 3);
	sprintf(tmpmsg, "%s: %s", msg, tmp);
	OutputDebugString(tmpmsg);
	free(tmpmsg);
	LocalFree(tmp);
}

void LoadFromFile(void)
{
	addNumberProc addNumber;
	HINSTANCE handle;
	DumpPEB();
	handle = LoadLibrary(DLL_FILE);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	DumpPEB();
	addNumber = (addNumberProc)GetProcAddress(handle, "addNumbers");
	printf("From file: %d\n", addNumber(1, 2));
	FreeLibrary(handle);
	DumpPEB();
}

void LoadFromMemory(void)
{
	FILE *fp;
	unsigned char *data=NULL;
	size_t size;
	HMEMORYMODULE module;
	addNumberProc addNumber;
	
	fp = fopen(DLL_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can't open DLL file \"%s\".", DLL_FILE);
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	data = (unsigned char *)malloc(size);
	fseek(fp, 0, SEEK_SET);
	fread(data, 1, size, fp);
	fclose(fp);

	DumpPEB();
	module = MemoryLoadLibrary(data, (unsigned char *)DLL_FILE);
	if (module == NULL)
	{
		printf("Can't load library from memory.\n");
		goto exit;
	}

	DumpPEB();
	addNumber = (addNumberProc)MemoryGetProcAddress(module, "addNumbers");
	if (addNumber)
		printf("From memory: %d\n", addNumber(1, 2));
	else
		printf("Not found\n");
	MemoryFreeLibrary(module);

exit:
	if (data)
		free(data);
}

int main(int argc, char* argv[])
{
	LoadFromFile();
	printf("\n\n");
	LoadFromMemory();
	return 0;
}

